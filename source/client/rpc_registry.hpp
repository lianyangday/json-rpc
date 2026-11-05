#include "requestor.hpp"

namespace Json_rpc
{
    namespace client
    {
        // 服务注册者
        class Provider
        {
        public:
            using ptr = std::shared_ptr<Provider>;
            Provider(const Requestor::ptr &requestor) : _requestor(requestor) {}

            // 方法注册
            bool RegisterMethod(const BaseConnection::ptr &con, const std::string &method, const Address &host)
            {
                ServiceRequest::ptr ser_req = MessageFactory::FactoryCreate<ServiceRequest>();
                ser_req->setId(UUID::uuid());
                ser_req->setMType(MType::REQ_SERVICE);
                ser_req->setmethod(method);
                ser_req->sethost(host);
                ser_req->setoptype(SERVICE_Optype::SERVICE_REGISTRY);
                BaseMessage::ptr msg_rsp;
                bool ret = _requestor->send(con, ser_req, msg_rsp);
                if (ret == false)
                {
                    ELOG("服务注册失败 %s 服务注册请求发送失败!", method.c_str());
                    return false;
                }

                // 类型转换***
                ServiceResponse::ptr ser_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                if (ser_rsp.get() == nullptr)
                {
                    ELOG("服务注册失败,相应类型向下转换失败!");
                    return false;
                }

                // response 的 rcode判断
                if (ser_rsp->rcode() != RCode::RCODE_SUCCESS)
                {
                    ELOG("服务注册失败,原因:%s", errReason(ser_rsp->rcode()).c_str());
                    return false;
                }

                return true;
            }

        private:
            Requestor::ptr _requestor;
        };

        class MethodHost
        {
        public:
            using ptr = std::shared_ptr<MethodHost>;
            MethodHost() {}

            MethodHost(const std::vector<Address> &hosts) : _hosts(hosts), _idx(0) {}

            void AppendHost(Address host)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _hosts.push_back(host);
            }
            // 采用负载均衡的取地址方式
            Address ChooseHost()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                size_t pos = _idx++ % _hosts.size();
                return _hosts[pos];
            }
            void RemoveHost(Address host)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _hosts.begin();
                for (; it < _hosts.end(); it++)
                {
                    if (*it == host)
                    {
                        _hosts.erase(it);
                        break;
                    }
                }
            }
            bool empty()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _hosts.empty();
            }

        private:
            std::mutex _mutex;
            size_t _idx;
            std::vector<Address> _hosts;
        };

        class Discoverer
        {
        public:
            using OfflineCallBack=std::function<void(const Address&)>;
            using ptr = std::shared_ptr<Discoverer>;

            Discoverer(const Requestor::ptr &requestor,OfflineCallBack cb) : _requestor(requestor),_offline_callback(cb) {}

            bool ServiceDiscover(const BaseConnection::ptr &con, const std::string &method, Address &host)
            {
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    // 发现服务
                    auto it = _method_hosts.find(method);
                    if (it != _method_hosts.end())
                    {
                        //_method_hosts表中有对应的服务方法,且存在服务方法的提供地址
                        if (it->second->empty() == false)
                        {
                            host = it->second->ChooseHost();
                            return true;
                        }
                    }
                }

                // 没有发现过服务方法&&发现过服务方法,没有有服务提供host
                // 向服务端请求
                ServiceRequest::ptr ser_req = MessageFactory::FactoryCreate<ServiceRequest>();
                ser_req->setId(UUID::uuid());
                ser_req->setMType(MType::REQ_SERVICE);
                ser_req->setmethod(method);
                ser_req->setoptype(SERVICE_Optype::SERVICE_DISCOVERY);
                BaseMessage::ptr msg_rsp;
                bool ret = _requestor->send(con, ser_req, msg_rsp);
                if (ret == false)
                {
                    ELOG("%s 服务发现失败!", method.c_str());
                    return false;
                }

                // 进行类型转换
                ServiceResponse::ptr ser_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                if (ser_rsp.get() == nullptr)
                {
                    ELOG("%s 服务发现失败,相应类型转换失败!", method.c_str());
                    return false;
                }

                // 进行消息rcode的判定
                if (ser_rsp->rcode() != RCode::RCODE_SUCCESS)
                {
                    ELOG("%s 服务发现失败! 原因:%s", method.c_str(), errReason(ser_rsp->rcode()).c_str());
                    return false;
                }

                // 申请的服务方法注册到_method_hosts中
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    std::vector<Address> hosts = ser_rsp->hosts();
                    MethodHost::ptr method_hosts = std::make_shared<MethodHost>(hosts);
                    // 向服务器请求服务,但是没有能够提供服务的主机
                    if (method_hosts->empty())
                    {
                        ELOG("%s 服务发现失败!没有能够提供服务的主机!", method.c_str());
                        return false;
                    }
                    _method_hosts[method] = method_hosts;
                    host = _method_hosts[method]->ChooseHost();
                    return true;
                }
            }

            // 注册进dispatcher服务中,用于自动处理serviceonline和serviceoffline服务
            void onServiceRequest(const BaseConnection::ptr &con, const ServiceRequest::ptr &ser_req)
            {

                std::string method = ser_req->method();
                Address host = ser_req->host();
                std::unique_lock<std::mutex> lock(_mutex);

                // 判断是服务上线通知还是服务下线通知
                // 服务上线通知
                if (ser_req->optype() == SERVICE_Optype::SERVICE_ONLINE)
                {
                    // 服务端发出的method是否与本客户端有关
                    auto it = _method_hosts.find(method);
                    if (it == _method_hosts.end())
                    {
                        // 曾经发现过服务,但是之前没有请求到服务提供地址
                        MethodHost::ptr method_hosts = std::make_shared<MethodHost>();
                        method_hosts->AppendHost(host);
                        _method_hosts[method] = method_hosts;
                    }
                    else
                    {
                        // 曾经发现过服务,同时收到过请求的服务提供地址
                        _method_hosts[method]->AppendHost(host);
                    }
                }
                // 服务下线通知
                else if (ser_req->optype() == SERVICE_Optype::SERVICE_OFFLINE)
                {
                    auto it = _method_hosts.find(method);
                    if (it != _method_hosts.end())
                    {
                        _method_hosts[method]->RemoveHost(host);
                        //删除的同时代用回调函数
                        _offline_callback(host);
                    }
                }
                // 未定义服务
                else
                {
                    ELOG("ServiceRequest中的服务不属于service_online和service_offline服务");
                }
            }

        private:
            std::mutex _mutex;
            OfflineCallBack _offline_callback;
            std::unordered_map<std::string, MethodHost::ptr> _method_hosts;
            Requestor::ptr _requestor;
        };

    }
}