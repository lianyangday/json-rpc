#pragma once
#include "abstract.hpp"
#include "message.hpp"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpClient.h>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace Json_rpc
{
    class ModuoBuffer : public BaseBuffer
    {
    public:
        using ptr = std::shared_ptr<ModuoBuffer>;
        ModuoBuffer(muduo::net::Buffer *bef) : _bef(bef) {}
        virtual size_t readableSize() override
        {
            return _bef->readableBytes();
        }
        virtual int32_t peekInt32() override
        {
            return _bef->peekInt32();
        }
        virtual int32_t readInt32() override
        {
            return _bef->readInt32();
        }
        virtual void retrieveInt32() override
        {
            _bef->retrieveInt32();
        }
        virtual std::string retrieveAsString(size_t len) override
        {
            return _bef->retrieveAsString(len);
        }

    private:
        muduo::net::Buffer *_bef;
    };

    // ModuoBuffer的工厂类
    class BufferFactory
    {
    public:
        template <class... Args>
        static ModuoBuffer::ptr create(Args &&...args)
        {
            return std::make_shared<ModuoBuffer>(std::forward<Args>(args)...);
        }
    };

    class LVProtocal : public Baseprotocal
    {
    public:
        using ptr = std::shared_ptr<LVProtocal>;
        // 信息格式
        //|--Len--|--Mtype--|--Idlen--|--Id--|--Body--|
        virtual bool canProcessed(const BaseBuffer::ptr &buffer) override
        {
            if(buffer->readableSize()<4)
            {
                return false;
            }
            int32_t total_len = buffer->peekInt32();
            size_t red_sum = buffer->readableSize();
            if (red_sum < (total_len + LenFiledLength))
            {
                return false;
            }
            return true;
        }

        // 在进行canprocessed成功的条件下
        virtual bool onMessageBuffer(const BaseBuffer::ptr &buffer, BaseMessage::ptr &msg) override
        {
            // readInt32()函数主动将网络字节序转化为主机字节序
            int32_t total_len = buffer->readInt32();
           //std::cout<<"total_len="<<total_len<<std::endl;
            MType mtype = (MType)buffer->readInt32();
            int32_t id_len = buffer->readInt32();
            int32_t body_len = total_len - MtypeFiledLength - IdFiledLength - id_len;
            std::string id = buffer->retrieveAsString(id_len);
            std::string body = buffer->retrieveAsString(body_len);
            msg = MessageFactory::FactoryCreate(mtype);
            if (msg.get() == nullptr)
            {
                ELOG("消息类型错误!\n");
                return false;
            }
            // 反序列化
            bool ret = msg->unserialize(body);
            if (ret == false)
            {
                ELOG("反序列化错误!\n");
                return false;
            }
            // 设置参数
            msg->setId(id);
            msg->setMType(mtype);
            return true;
        }

        virtual std::string serialize(const BaseMessage::ptr &msg) override
        {
            // 大小端：在多个字节组成的数据类型中，将低位字节放在起始地址称为小端，将高位字节放在起始地址成为大端
            // 网络字节序时大端
            // 需要将主机字节序转化为网络字节序
            std::string body = msg->serialize();
            int32_t body_len = body.size();

            std::string id = msg->id();
            int32_t id_len = htonl(id.size());
            auto mtype = htonl((int32_t)msg->mtype());
            int32_t total_h_len=MtypeFiledLength + IdFiledLength + id.size() + body_len;
            int32_t total_l_len = htonl(total_h_len);
            std::string result;
            result.reserve(LenFiledLength + total_h_len);
            result.append((char *)&total_l_len, LenFiledLength);
            result.append((char *)&mtype, MtypeFiledLength);
            result.append((char *)&id_len, IdFiledLength);
            result.append(id);
            result.append(body);
            return result;
        }

    private:
        const size_t LenFiledLength = 4;
        const size_t MtypeFiledLength = 4;
        const size_t IdFiledLength = 4;
    };

    class ProtocalFactory
    {
    public:
        template <class... Args>
        static LVProtocal::ptr create(Args &&...args)
        {
            return std::make_shared<LVProtocal>(std::forward<Args>(args)...);
        }
    };

    class ModuoConnnection : public BaseConnection
    {
    public:
        using ptr = std::shared_ptr<ModuoConnnection>;
        // use Baseprotocal::ptr here so connection can work with any protocol implementation
        ModuoConnnection(const Baseprotocal::ptr &protocal, const muduo::net::TcpConnectionPtr &con) : _protocal(protocal), _con(con) {}
        virtual void send(const BaseMessage::ptr &msg) override
        {
            std::string message = _protocal->serialize(msg);
            _con->send(message);
        }
        virtual void shutdown() override
        {
            _con->shutdown();
        }
        virtual bool connected() override
        {
            return _con->connected();
        }

    private:
        Baseprotocal::ptr _protocal;
        muduo::net::TcpConnectionPtr _con;
    };

    class ConnnectionFactory
    {
    public:
        template <class... Args>
        static ModuoConnnection::ptr create(Args &&...args)
        {
            return std::make_shared<ModuoConnnection>(std::forward<Args>(args)...);
        }
    };

    class MuduoServer : public BaseServer
    {
    public:
        MuduoServer(int port)
            : _server(&_loop, muduo::net::InetAddress("0.0.0.0", port), "MuduoServer", muduo::net::TcpServer::kReusePort),
              _protocal(ProtocalFactory::create())
        {
        }

        void start() override
        {
            _server.setConnectionCallback(std::bind(&MuduoServer::OnConnection, this, std::placeholders::_1));
            _server.setMessageCallback(std::bind(&MuduoServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            // 开始和监听
            _server.start();
            _loop.loop();
        }

        void OnConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                DLOG("连接建立!\n");
                ModuoConnnection::ptr moduo_conn = ConnnectionFactory::create(_protocal, conn);
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    // use operator[] to insert or overwrite existing mapping
                    _conns[conn] = moduo_conn;
                }

                if (_cb_connection)
                    _cb_connection(moduo_conn);
            }
            else
            {
                ELOG("连接断开!\n");
                BaseConnection::ptr base_conn;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if (it != _conns.end())
                    {
                        base_conn = it->second;
                        _conns.erase(it);
                    }
                }
                if (_cb_close)
                    _cb_close(base_conn);
            }
        }

        void OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
        {
            DLOG("有消息传来，开始处理!\n");
            ModuoBuffer::ptr buffer = BufferFactory::create(buf);
            while (1)
            {
                if (_protocal->canProcessed(buffer) == false)
                {

                    // 不能读取一条完整的数据
                    // 情况一：数据过大
                    if (buffer->readableSize() > maxdatasize)
                    {
                        conn->shutdown();
                        ELOG("数据超过数据缓存区!\n");
                        return;
                    }
                    // 情况二：
                    // 数据量合理，但是不足
                    break;
                }
                else
                {

                    BaseMessage::ptr msg;
                    bool ret = _protocal->onMessageBuffer(buffer, msg);
                    if (ret == false)
                    {
                        conn->shutdown();
                        ELOG("缓冲区数据错误!\n");
                        return;
                    }
                    BaseConnection::ptr base_conn;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(conn);
                        if (it == _conns.end())
                        {
                            conn->shutdown();
                            ELOG("未建立连接!\n");
                            return;
                        }
                        base_conn = it->second;
                    }
                    DLOG("数据处理完毕，调用回调函数进行处理!\n");
                    if (_cb_message)
                    {
                        _cb_message(base_conn, msg);

                    }
                }
            }
        }

        using ptr = std::shared_ptr<MuduoServer>;
        // start() is implemented above; no pure virtual here

    protected:
        const size_t maxdatasize = (1 << 16); // 2的16次方
        muduo::net::TcpServer _server;
        muduo::net::EventLoop _loop;
        LVProtocal::ptr _protocal;
        // map:将TcpConnectionPtr和ModuoConnnection相关联
        std::unordered_map<muduo::net::TcpConnectionPtr, BaseConnection::ptr> _conns;
        // map为共享资源，需要使用锁进行保护
        std::mutex _mutex;
    };

    class ServerFactory
    {
    public:
        template <typename... Args>
        static BaseServer::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
        }
    };

    class MuduoClient : public BaseClient
    {
    public:
        using ptr = std::shared_ptr<MuduoClient>;
        MuduoClient(const std::string &sip, int sport) : _protocol(ProtocalFactory::create()),
                                                         _loop(_loopthread.startLoop()),
                                                         _downlatch(1),
                                                         _client(_loop, muduo::net::InetAddress(sip, sport), "MuduoClient") {}

        virtual void connect() override
        {
            DLOG("设置回调函数，连接服务器");
            _client.setConnectionCallback(std::bind(&MuduoClient::onConnection, this, std::placeholders::_1));
            // 设置连接消息的回调
            _client.setMessageCallback(std::bind(&MuduoClient::onMessage, this,
                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // 连接服务器
            _client.connect();

            //阻塞等待连接成功
            _downlatch.wait();
            DLOG("连接服务器成功！");
        }

        virtual void shutdown() override
        {
            return _client.disconnect();
        }

        virtual bool send(const BaseMessage::ptr &msg) override
        {
            DLOG("发送消息到服务器！");
            if (connected() == false)
            {
                ELOG("连接已断开！");
                return false;
            }
            _conn->send(msg);
            return true;
        }

        virtual BaseConnection::ptr connection() override
        {
            return _conn;
        }

        virtual bool connected()
        {
            return (_conn && _conn->connected());
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                std::cout << "连接建立!\n";
                //连接成功,调用回调函数,取消阻塞
                _downlatch.countDown(); // 计数--，为0时唤醒阻塞
                // create connection with protocol first (matches ModuoConnnection ctor)
                _conn = ConnnectionFactory::create(_protocol, conn);
            }
            else
            {
                std::cout << "连接断开!\n";
                _conn.reset();
            }
        }
        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
        {
            DLOG("连接有数据到来，开始处理！");
            auto base_buf = BufferFactory::create(buf);
            while (1)
            {
                if (_protocol->canProcessed(base_buf) == false)
                {
                    // 数据不足
                    if (base_buf->readableSize() > maxDataSize)
                    {
                        conn->shutdown();
                        ELOG("缓冲区中数据过大！");
                        return;
                    }
                    // DLOG("数据量不足！");
                    break;
                }
                // DLOG("缓冲区中数据可处理！");
                BaseMessage::ptr msg;
                bool ret = _protocol->onMessageBuffer(base_buf, msg);
                if (ret == false)
                {
                    conn->shutdown();
                    ELOG("缓冲区中数据错误！");
                    return;
                }
                if(msg.get()==nullptr)
                {
                    ELOG("数据为空");
                }
                // DLOG("缓冲区中数据解析完毕，调用回调函数进行处理！");
                if (_cb_message)
                    _cb_message(_conn, msg);
            }
        }

    private:
        const size_t maxDataSize = (1 << 16);
        Baseprotocal::ptr _protocol;
        BaseConnection::ptr _conn;
        muduo::CountDownLatch _downlatch;
        muduo::net::EventLoopThread _loopthread;
        muduo::net::EventLoop *_loop;
        muduo::net::TcpClient _client;
    };

    class ClientFactory
    {
    public:
        template <typename... Args>
        static BaseClient::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
        }
    };

}