#pragma once
#include "../common/net.hpp"
#include "../common/message.hpp"
#include <set>
namespace Json_rpc
{
    namespace server
    {
        class ProviderManager
        {
        public:
            using ptr = std::shared_ptr<ProviderManager>;
            class Provider
            {
            public:
                using ptr = std::shared_ptr<Provider>;
                // 初始化
                Provider(const BaseConnection::ptr &con, const Address &host) : _con(con), _host(host) {}
                // 服务注册者添加方法
                void AppendMethod(const std::string &method)
                {

                    std::unique_lock<std::mutex> lock(_mutex);
                    _methods.emplace_back(method);
                }
                std::vector<std::string> _methods;
                std::mutex _mutex;
                BaseConnection::ptr _con;
                Address _host;
            };

            // 新增服务注册者&&服务者添加新的方法
            Provider::ptr AddProvider(const BaseConnection::ptr &con, const Address &host, const std::string &method)
            {
                Provider::ptr provider;
                {
                    // 共享资源需要加锁进行保护
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conn.find(con);
                    if (it != _conn.end())
                    {
                        // 已存在,不需要注册
                        provider = it->second;
                    }
                    else
                    {
                        provider = std::make_shared<Provider>(con, host);
                        // 进行注册
                        _conn.insert(std::make_pair(con, provider));
                    }
                    _providers[method].insert(provider);
                }
                provider->AppendMethod(method);
                return provider;
            }
            // 删除服务注册者
            void DelProvider(const BaseConnection::ptr &con)
            {
                Provider::ptr provider;
                // 删除服务注册者关联的所有方法
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conn.find(con);
                    if (it == _conn.end())
                    {
                        return;
                    }
                    provider = it->second;
                    for (auto &method : provider->_methods)
                    {
                        _providers[method].erase(provider);
                    }
                }
            }
            // 获取服务注册者
            Provider::ptr GetProvider(const BaseConnection::ptr &con)
            {
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conn.find(con);
                    if (it == _conn.end())
                    {
                        return Provider::ptr();
                    }
                    return _conn[con];
                }
            }

            std::vector<Address> MethodHosts(const std::string &method)
            {
                auto it = _providers.find(method);
                if (it == _providers.end())
                {
                    return std::vector<Address>();
                }
                std::vector<Address> address;
                for(auto&e:it->second)
                {
                    address.push_back(e->_host);
                }
                return  address;
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, std::set<Provider::ptr>> _providers;
            std::unordered_map<BaseConnection::ptr, Provider::ptr> _conn;
        };

        class DiscovererManager
        {
        public:
            using ptr = std::shared_ptr<DiscovererManager>;
            class Discoverer
            {
            public:
                using ptr = std::shared_ptr<Discoverer>;
                // 初始化
                Discoverer(const BaseConnection::ptr &con) : _con(con) {}
                // 服务注册者添加方法
                void AppendMethod(const std::string &method)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _methods.emplace_back(method);
                }

                std::vector<std::string> _methods;
                std::mutex _mutex;
                BaseConnection::ptr _con;
            };
            // 新增服务发现者
            Discoverer::ptr AddDiscover(const BaseConnection::ptr &con, const std::string &method)
            {
                Discoverer::ptr discover;
                {
                    // 共享资源需要加锁进行保护
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conn.find(con);
                    if (it != _conn.end())
                    {
                        // 已存在,不需要注册
                        discover = it->second;
                    }
                    else
                    {
                        discover = std::make_shared<Discoverer>(con);
                        // 进行注册
                        _conn.insert(std::make_pair(con, discover));
                    }
                    _discovers[method].insert(discover);
                }
                discover->AppendMethod(method);
                return discover;
            }
            // 删除服务发现者
            void DelDiscover(const BaseConnection::ptr &con)
            {
                Discoverer::ptr discover;
                // 删除服务注册者关联的所有方法
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conn.find(con);
                    if (it == _conn.end())
                    {
                        return;
                    }
                    discover = it->second;
                    for (auto &method : discover->_methods)
                    {
                        _discovers[method].erase(discover);
                    }
                }
            }
            // 新服务者上线通知
            void OnlineNotify(const std::string &method, const Address &host)
            {
                notify(method, host, SERVICE_Optype::SERVICE_ONLINE);
            }
            // 服务下线通知
            void OfflineNotify(const std::string &method, const Address &host)
            {
                notify(method, host, SERVICE_Optype::SERVICE_OFFLINE);
            }

        private:
            void notify(const std::string &method, const Address &host, SERVICE_Optype optype)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _discovers.find(method);
                if (it == _discovers.end())
                {
                    return;
                }
                // 通知服务下线
                auto ser_req = MessageFactory::FactoryCreate<ServiceRequest>();
                ser_req->setId(UUID::uuid());
                ser_req->setmethod(method);
                ser_req->setMType(MType::REQ_SERVICE);
                ser_req->setoptype(optype);
                ser_req->sethost(host);

                for (auto &discovers : _discovers[method])
                {
                    discovers->_con->send(ser_req);
                }
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, std::set<Discoverer::ptr>> _discovers;
            std::unordered_map<BaseConnection::ptr, Discoverer::ptr> _conn;
        };

        class PDManager
        {
        public:
            using ptr = std::shared_ptr<PDManager>;
            PDManager() : _providermanager(std::make_shared<ProviderManager>()), _discoverermanager(std::make_shared<DiscovererManager>())
            {
            }
            void OnServerRequest(const BaseConnection::ptr &con, const ServiceRequest::ptr &ser_req)
            {
                SERVICE_Optype optype = ser_req->optype();
                // 服务注册者上线
                if (optype == SERVICE_Optype::SERVICE_REGISTRY)
                {
                    DLOG("服务注册者上线");
                    _providermanager->AddProvider(con, ser_req->host(), ser_req->method());
                    // 服务上线通知
                    _discoverermanager->OnlineNotify(ser_req->method(), ser_req->host());

                    // 需要对服务注册者进行通知
                    return ProviderResponse(con, ser_req);
                }
                // 服务发现者上线
                else if (optype == SERVICE_Optype::SERVICE_DISCOVERY)
                {
                    DLOG("服务发现者上线");
                    _discoverermanager->AddDiscover(con, ser_req->method());
                    return DiscoverResponse(con, ser_req);
                }
                else
                {
                    ELOG("收到服务操作请求,但是服务操作类型错误!");
                    return ErrorResponse(con, ser_req);
                }
            }

            void ShutConnection(const BaseConnection::ptr &con)
            {
                // 服务注册者下线
                auto provider = _providermanager->GetProvider(con);
                if (provider.get() != nullptr)
                {
                    // 服务注册者下线通知
                    for (auto &method : provider->_methods)
                    {
                        _discoverermanager->OfflineNotify(method, provider->_host);
                    }
                    _providermanager->DelProvider(con);
                }
                _discoverermanager->DelDiscover(con);
                // 服务发现这下线
            }

        private:
            void ErrorResponse(const BaseConnection::ptr &con, const ServiceRequest::ptr &ser_req)
            {
                ServiceResponse::ptr ser_rsp = MessageFactory::FactoryCreate<ServiceResponse>();
                ser_rsp->setId(ser_req->id());
                ser_rsp->setMType(MType::RSP_SERVICE);
                ser_rsp->setmethod(ser_req->method());
                ser_rsp->setoptype(SERVICE_Optype::SERVICE_UNKNOW);
                ser_rsp->setrcode(RCode::RCODE_INVALID_OPTYPE);
                return con->send(ser_rsp);
            }

            void ProviderResponse(const BaseConnection::ptr &con, const ServiceRequest::ptr &ser_req)
            {
                ServiceResponse::ptr ser_rsp = MessageFactory::FactoryCreate<ServiceResponse>();
                ser_rsp->setId(ser_req->id());
                ser_rsp->setMType(MType::RSP_SERVICE);
                ser_rsp->setmethod(ser_req->method());
                ser_rsp->setoptype(SERVICE_Optype::SERVICE_REGISTRY);
                ser_rsp->setrcode(RCode::RCODE_SUCCESS);
                return con->send(ser_rsp);
            }

            void DiscoverResponse(const BaseConnection::ptr &con, const ServiceRequest::ptr &ser_req)
            {
                std::vector<Address> hosts;
                ServiceResponse::ptr ser_rsp = MessageFactory::FactoryCreate<ServiceResponse>();
                ser_rsp->setId(ser_req->id());
                ser_rsp->setmethod(ser_req->method());
                ser_rsp->setMType(MType::RSP_SERVICE);
                ser_rsp->setoptype(SERVICE_Optype::SERVICE_DISCOVERY);
                hosts = _providermanager->MethodHosts(ser_req->method());
                if (hosts.empty()==true)
                {
                    ser_rsp->setrcode(RCode::RCODE_NOT_FOUND_RPCSERVICE);
                    return con->send(ser_rsp);
                }
                ser_rsp->sethosts(hosts);
                ser_rsp->setrcode(RCode::RCODE_SUCCESS);
                return con->send(ser_rsp);
            }

        private:
            ProviderManager::ptr _providermanager;
            DiscovererManager::ptr _discoverermanager;
        };
    }
}
