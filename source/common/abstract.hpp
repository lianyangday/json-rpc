#pragma once
#include<iostream>
#include<string>
#include<memory>
#include<functional>

#include"fields.hpp"


namespace Json_rpc
{
    //抽象缓冲区类
    class BaseBuffer
    {
        public:
        using ptr=std::shared_ptr<BaseBuffer>;
        virtual size_t readableSize()=0;
        virtual int32_t peekInt32()=0;
        virtual int32_t readInt32()=0;
        virtual void retrieveInt32()=0;
        virtual std::string retrieveAsString(size_t len)=0;

    };



    //抽象消息类
    class BaseMessage
    {
        public:
        using ptr=std::shared_ptr<BaseMessage>;
        virtual ~BaseMessage() {};
        virtual void setMType(MType type)
        {
            _type=type;
        };
        virtual void setId(const std::string &id)
        {
            _id=id;
        }
        virtual MType mtype()
        {
            return _type;
        }
        virtual std::string id()
        {
            return _id;
        }
        virtual std::string serialize()=0;
        virtual bool unserialize(const std::string&body)=0;
        virtual bool check()=0;
        
        protected:
        MType _type;
        std::string _id;
    };

    //抽象网络连接类
    class BaseConnection
    {
        public:
        using ptr=std::shared_ptr<BaseConnection>;
        virtual void send(const BaseMessage::ptr&msg)=0;
        virtual void shutdown()=0;
        virtual bool connected()=0;
    };

    //抽象基础协议处理类
    class Baseprotocal
    {
        public:
        using ptr=std::shared_ptr<Baseprotocal>;
        virtual bool onMessageBuffer(const BaseBuffer::ptr&buffer,BaseMessage::ptr&msg)=0;
        virtual std::string serialize(const BaseMessage::ptr&msg)=0;
        virtual bool canProcessed(const BaseBuffer::ptr &buffer)=0;
    };

    using ConnectionCallback=std::function<void(const BaseConnection::ptr&)>;
    using CloseCallback=std::function<void(const BaseConnection::ptr&)>;
    using MessageCallback=std::function<void(const BaseConnection::ptr&,BaseMessage::ptr&)>;

    //抽象服务端类
    class BaseServer
    {
        public:
        using ptr=std::shared_ptr<BaseServer>;
        virtual void setConnetionCallback(const ConnectionCallback &cb_connection)
        {
            _cb_connection=cb_connection;
        }

         virtual void setCloseCallback(const CloseCallback &cb_close)
        {
            _cb_close=cb_close;
        } 
        
        virtual void setMessageCallback(const MessageCallback &cb_message)
        {
            _cb_message=cb_message;
        }

        virtual void start()=0;


        protected:
        ConnectionCallback _cb_connection;
        CloseCallback _cb_close;
        MessageCallback _cb_message;
    };

    //抽象客户端类
    class BaseClient
    {
        public:
        using ptr=std::shared_ptr<BaseClient>;
        virtual void setConnetionCallback(const ConnectionCallback &cb_connection)
        {
            _cb_connection=cb_connection;
        }

         virtual void setCloseCallback(const CloseCallback &cb_close)
        {
            _cb_close=cb_close;
        } 
        
        virtual void setMessageCallback(const MessageCallback &cb_message)
        {
            _cb_message=cb_message;
        }

        virtual void connect()=0;
        virtual void shutdown()=0;
        virtual bool send(const BaseMessage::ptr &msg)=0;
        virtual bool connected()=0;
        virtual BaseConnection::ptr connection()=0;

        protected:
        ConnectionCallback _cb_connection;
        CloseCallback _cb_close;
        MessageCallback _cb_message;

    };
}