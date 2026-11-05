#pragma once
#include <iostream>
#include "../common/dispatcher.hpp"
#include "../common/net.hpp"
#include "rpc_caller.hpp"
#include "rpc_registry.hpp"
#include "rpc_topic.hpp"

namespace Json_rpc
{
    namespace client
    {
        // 客户端分为三种
        // 1.服务注册客户端
        // 2.服务发现客户端
        // 3.普通的rpc客户端,分为基于服务注册的rpc客户端和不基于服务注册的rpc客户端

        class ProviderClient
        {
        public:
            using ptr = std::shared_ptr<ProviderClient>;
            ProviderClient(const std::string &ip, const int port)
                : _requestor(std::make_shared<Requestor>()),
                  _provider(std::make_shared<client::Provider>(_requestor)),
                  _dispatcher(std::make_shared<Dispatcher>())
            {
                auto cb = std::bind(&client::Requestor::onResponse, _requestor.get(), std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerhandler<ServiceResponse>(MType::RSP_SERVICE, cb);

                _client = ClientFactory::create(ip, port);
                auto onmessagecallback = std::bind(&Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                _client->setMessageCallback(onmessagecallback);
                _client->connect();
            }
            // 服务注册
            bool RegisterMethod(const std::string &method, const Address &host)
            {
                return _provider->RegisterMethod(_client->connection(), method, host);
            }

        private:
            Requestor::ptr _requestor;
            client::Provider::ptr _provider;
            Dispatcher::ptr _dispatcher;
            BaseClient::ptr _client;
        };

        class DiscoverClient
        {
        public:
            using ptr = std::shared_ptr<DiscoverClient>;
            DiscoverClient(const std::string &ip, const int port, Discoverer::OfflineCallBack cb)
                : _requestor(std::make_shared<Requestor>()),
                  _discover(std::make_shared<client::Discoverer>(_requestor, cb)),
                  _dispatcher(std::make_shared<Dispatcher>())
            {
                auto rsp_service_cb = std::bind(&client::Requestor::onResponse, _requestor.get(), std::placeholders::_1, std::placeholders::_2);
                auto req_service_cb = std::bind(&client::Discoverer::onServiceRequest, _discover.get(), std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerhandler<ServiceResponse>(MType::RSP_SERVICE, rsp_service_cb);
                _dispatcher->registerhandler<ServiceRequest>(MType::REQ_SERVICE, req_service_cb);

                _client = ClientFactory::create(ip, port);
                auto onmessagecallback = std::bind(&Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                _client->setMessageCallback(onmessagecallback);
                _client->connect();
            }
            // 服务发现
            bool ServiceDiscover(const std::string &method, Address &host)
            {
                return _discover->ServiceDiscover(_client->connection(), method, host);
            }

        private:
            Requestor::ptr _requestor;
            client::Discoverer::ptr _discover;
            Dispatcher::ptr _dispatcher;
            BaseClient::ptr _client;
        };

        class TopicClient
        {
        public:
            using ptr = std::shared_ptr<TopicClient>;
            TopicClient(const std::string &ip, const int port)
                : _requestor(std::make_shared<Requestor>()),
                  _topic_manager(std::make_shared<client::TopicManager>(_requestor)),
                  _dispatcher(std::make_shared<Dispatcher>())
            {
                auto rsp_topic_cb = std::bind(&client::Requestor::onResponse, _requestor.get(), std::placeholders::_1, std::placeholders::_2);
                auto rep_topic_cb = std::bind(&client::TopicManager::OnPublish, _topic_manager.get(), std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerhandler<TopicResponse>(MType::RSP_TOPIC, rsp_topic_cb);
                _dispatcher->registerhandler<TopicRequest>(MType::REQ_TOPIC, rep_topic_cb);

                _client = ClientFactory::create(ip, port);
                auto onmessagecallback = std::bind(&Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                _client->setMessageCallback(onmessagecallback);
                _client->connect();
            }

            bool CreateTopic(const std::string &key)
            {
                return _topic_manager->CreateTopic(_client->connection(), key);
            }

            bool RemoveTopic(const std::string &key)
            {
                return _topic_manager->RemoveTopic(_client->connection(), key);
            }

            bool SubcribeTopic(const std::string &key, const TopicManager::SubCallback &sub_cb)
            {
                return _topic_manager->SubcribeTopic(_client->connection(), key, sub_cb);
            }

            bool CancelTopic(const std::string &key)
            {
                return _topic_manager->CancelTopic(_client->connection(), key);
            }

            bool PublishTopic(const std::string &key, const std::string &msg)
            {
                return _topic_manager->PublishTopic(_client->connection(), key,msg);
            }

        private:
            Requestor::ptr _requestor;
            client::TopicManager::ptr _topic_manager;
            Dispatcher::ptr _dispatcher;
            BaseClient::ptr _client;
        };

        class RpcClient
        {
        public:
            using ptr = std::shared_ptr<RpcClient>;
            // 使用enablediscover表示,RpcClient是否是基于服务发现的功能客户端
            // enablediscover:true,说明ip和port是服务注册服务器的
            // enablediscover:false,说明ip和port是服务提供服务器的,直接基于RpcCaller直接可服务提供者通信
            RpcClient(bool enablediscover, const std::string &ip, const int port)
                : _enablediscover(enablediscover),
                  _requestor(std::make_shared<Requestor>()),
                  _dispatcher(std::make_shared<Dispatcher>()),
                  _rpccaller(std::make_shared<RpcCaller>(_requestor))
            {

                auto cb = std::bind(&client::Requestor::onResponse, _requestor.get(), std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerhandler<RpcResponse>(MType::RSP_RPC, cb);

                if (_enablediscover)
                {
                    auto cb = std::bind(&RpcClient::DelClient, this, std::placeholders::_1);
                    _discover = std::make_shared<DiscoverClient>(ip, port, cb);
                }

                _rpc_client = ClientFactory::create(ip, port);
                auto onmessagecallback = std::bind(&Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                _rpc_client->setMessageCallback(onmessagecallback);
                _rpc_client->connect();
            }

            bool Call(const std::string &method, const Json::Value &params, Json::Value &result)
            {
                BaseClient::ptr client;
                client = GetClient(method);
                if (client.get() == nullptr)
                {
                    ELOG("%s 服务失败,没有服务器提供服务!", method.c_str());
                    return false;
                }
                _rpccaller->Call(client->connection(), method, params, result);
                return true;
            }
            bool Call(const std::string &method, const Json::Value &params, RpcCaller::JsonAsyncResponse &result)
            {
                BaseClient::ptr client;
                client = GetClient(method);
                if (client.get() == nullptr)
                {
                    ELOG("%s 服务失败,没有服务器提供服务!", method.c_str());
                    return false;
                }
                _rpccaller->Call(client->connection(), method, params, result);
                return true;
            }
            bool Call(const std::string &method, const Json::Value &params, const RpcCaller::JsonRequestCallBack &jsonreqcallback)
            {
                BaseClient::ptr client;
                client = GetClient(method);
                if (client.get() == nullptr)
                {
                    ELOG("%s 服务失败,没有服务器提供服务!", method.c_str());
                    return false;
                }
                _rpccaller->Call(client->connection(), method, params, jsonreqcallback);
                return true;
            }

        private:
            struct AddressHost
            {
                size_t operator()(const Address &host) const
                {
                    std::string addr = host.first + std::to_string(host.second);
                    return std::hash<std::string>{}(addr);
                }
            };

        private:
            BaseClient::ptr GetClient(const Address &host)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _rpc_clients.find(host);
                if (it != _rpc_clients.end())
                {
                    return it->second;
                }

                return BaseClient::ptr();
            }

            BaseClient::ptr GetClient(const std::string &method)
            {
                BaseClient::ptr client;
                if (_enablediscover)
                {
                    // 进行服务发现
                    Address host;
                    bool ret = _discover->ServiceDiscover(method, host);
                    if (ret == false)
                    {
                        return BaseClient::ptr();
                    }

                    client = GetClient(host);
                    if (client.get() == nullptr)
                    {
                        client = AddClient(host);
                    }
                }
                else
                {
                    client = _rpc_client;
                }
                return client;
            }

            void PutClient(const Address &host, const BaseClient::ptr &client)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _rpc_clients.insert(std::make_pair(host, client));
            }

            BaseClient::ptr AddClient(const Address &host)
            {
                auto client = ClientFactory::create(host.first, host.second);
                auto onmessagecallback = std::bind(&Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                client->setMessageCallback(onmessagecallback);
                client->connect();
                PutClient(host, client);
                return client;
            }

            void DelClient(const Address &host)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _rpc_clients.erase(host);
            }

        private:
            bool _enablediscover;
            Requestor::ptr _requestor;
            RpcCaller::ptr _rpccaller;
            client::DiscoverClient::ptr _discover;
            Dispatcher::ptr _dispatcher;
            BaseClient::ptr _rpc_client; // 直接连接服务提供者
            std::mutex _mutex;
            std::unordered_map<Address, BaseClient::ptr, AddressHost> _rpc_clients; // 直接连接服务发现者
        };
    }
}