#pragma once
#include "../client/rpc_client.hpp"
#include "rpc_registry.hpp"
#include "rpc_router.hpp"
#include "../common/dispatcher.hpp"
#include "rpc_topic.hpp"

namespace Json_rpc
{
    namespace server
    {
        // 有两种服务器
        // 1.服务注册中心服务端
        // 2.不拥有服务注册客户端的服务器

        class RegistryServer
        {
        public:
            using ptr = std::shared_ptr<RegistryServer>;
            RegistryServer(const std::string &ip, int port)
                : _dispatcher(std::make_shared<Dispatcher>()),
                  _pdmanager(std::make_shared<PDManager>())
            {
                _server = ServerFactory::create(port);
                auto ser_cb = std::bind(&PDManager::OnServerRequest, _pdmanager.get(), std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerhandler<Json_rpc::ServiceRequest>(Json_rpc::MType::REQ_SERVICE, ser_cb);
                // server连接时.调用的回调函数
                auto onmessagecallback = std::bind(&Json_rpc::Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                _server->setMessageCallback(onmessagecallback);
                // 对应的连接关闭时,调用的回调函数
                auto close_cb = std::bind(&PDManager::ShutConnection, _pdmanager.get(), std::placeholders::_1);
                _server->setCloseCallback(close_cb);
            };

            void start()
            {
                _server->start();
            }

        private:
            BaseServer::ptr _server;
            Dispatcher::ptr _dispatcher;
            PDManager::ptr _pdmanager;
        };

        class TopicServer
        {
        public:
            using ptr = std::shared_ptr<TopicServer>;
            TopicServer( int port)
                : _dispatcher(std::make_shared<Dispatcher>()),
                  _topmanager(std::make_shared<TopicManager>())
            {
                _server = ServerFactory::create(port);
                auto ser_cb = std::bind(&TopicManager::OnTopicRequest, _topmanager.get(), std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerhandler<Json_rpc::TopicRequest>(Json_rpc::MType::REQ_TOPIC, ser_cb);
                // server连接时,有消息传入后.调用的回调函数
                auto onmessagecallback = std::bind(&Json_rpc::Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                _server->setMessageCallback(onmessagecallback);
                // 对应的连接关闭时,调用的回调函数
                auto close_cb = std::bind(&TopicManager::OnShutdown, _topmanager.get(), std::placeholders::_1);
                _server->setCloseCallback(close_cb);
            };

            void start()
            {
                _server->start();
            }

        private:
            BaseServer::ptr _server;
            Dispatcher::ptr _dispatcher;
            TopicManager::ptr _topmanager;
        };

        class RpcServer
        {
        public:
            using ptr = std::shared_ptr<RpcServer>;
            RpcServer(const Address &access, bool enableregistry = false, const Address &registry_address = Address())
                : _enableregistry(enableregistry),
                  _access(access),
                  _rpc_router(std::make_shared<RpcRouter>()),
                  _dispatcher(std::make_shared<Dispatcher>())
            {
                if (_enableregistry)
                {
                    // 初始化服务注册客户端
                    _prc_registry_client = std::make_shared<client::ProviderClient>(registry_address.first, registry_address.second);
                }

                _server = ServerFactory::create(access.second);
                auto rpc_cb = std::bind(&RpcRouter::OnRpcRequest, _rpc_router.get(), std::placeholders::_1, std::placeholders::_2);
                _dispatcher->registerhandler<Json_rpc::RpcRequest>(Json_rpc::MType::REQ_RPC, rpc_cb);
                auto onmessagecallback = std::bind(&Json_rpc::Dispatcher::onmessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
                _server->setMessageCallback(onmessagecallback);
            }

            void RegisterMethod(const ServiceDescribe::ptr &service)
            {
                // 进行服务注册&&向注册中心进行服务注册
                _rpc_router->RegisterMethod(service);
                if (_enableregistry)
                {
                    _prc_registry_client->RegisterMethod(service->methodName(), _access);
                }
            }

            void start()
            {
                _server->start();
            }

        private:
            bool _enableregistry;
            // 自己的主机地址
            Address _access;
            BaseServer::ptr _server;
            client::ProviderClient::ptr _prc_registry_client;
            RpcRouter::ptr _rpc_router;
            Dispatcher::ptr _dispatcher;
        };
    }
}