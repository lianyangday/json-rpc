#pragma once
#include "../common/net.hpp"
#include "../common/message.hpp"
namespace Json_rpc
{
    namespace server
    {
        // json中的数据类型
        enum Type
        {
            BOOL = 0,
            INTEGRAL,
            STRING,
            ARRAY,
            OBJECT
        };

        class ServiceDescribe
        {
        public:
            using ptr = std::shared_ptr<ServiceDescribe>;
            using ParamType = std::pair<std::string, Type>;
            using ServerCallBack = std::function<void(const Json::Value &, Json::Value &)>;

            ServiceDescribe( std::string &&method_name,
                             std::vector<ParamType> &&param_types,
                            Type return_type,
                             ServerCallBack &&callback) : _method_name(std::move(method_name)),
                                                               _param_types(std::move(param_types)),
                                                               _return_type(return_type),
                                                               _callback(std::move(callback))
            {
            }

            bool paramCheck(const Json::Value &params)
            {
                // 使用参数名进行参数的check
                for (auto &desc : _param_types)
                {
                    if (params.isMember(desc.first) == false)
                    {
                        ELOG("参数缺失: %s\n", desc.first.c_str());
                        return false;
                    }
                    // 参数类型检查
                    if (checkType(desc.second, params[desc.first]) == false)
                    {
                        ELOG("参数类型错误: %s\n", desc.first.c_str());
                        return false;
                    }
                }
                return true;
            };

            bool call(const Json::Value &val, Json::Value &result)
            {
                _callback(val, result);
                if (resultCheck(result) == false)
                {
                    ELOG("回调函数返回值类型错误!\n");
                    return false;
                }
                return true;
            }

            std::string methodName()
            {
                return _method_name;
            }

        private:
            // 参数类型的检查
            bool checkType(Type type, const Json::Value &params)
            {
                switch (type)
                {
                case BOOL:
                    return params.isBool();
                case INTEGRAL:
                    return params.isInt();
                case STRING:
                    return params.isString();
                case ARRAY:
                    return params.isArray();
                case OBJECT:
                    return params.isObject();
                }
                return false;
            }

            bool resultCheck(const Json::Value &result)
            {
                // 返回值类型检查
                return checkType(_return_type, result);
            }

        private:
            // RPC服务方法名,
            std::string _method_name;
            // RPC服务方法参数列表
            std::vector<ParamType> _param_types;
            // RPC服务方法返回值类型
            Type _return_type;
            // RPC服务方法回调函数
            ServerCallBack _callback;
        };

        // 创造者类      :创造者类是用于保护对象内容，在创造者类中初始化，初始化之后的类型不可更改
        class ServiceDescribeFactory
        {
        public:
            using ParamType = std::pair<std::string, Type>;
            using ServerCallBack = std::function<void(const Json::Value &, Json::Value &)>;
            void set_menthod_name(const std::string &method_name)
            {
                _method_name = method_name;
            }

            void set_param_types(const std::string &param_types, Type type)
            {
                _param_types.push_back(std::make_pair(param_types, type));
            }

            void set_return_type(Type return_type)
            {
                _return_type = return_type;
            }

            void set_callback(const ServerCallBack &callback)
            {
                _callback = callback;
            }

            ServiceDescribe::ptr create()
            {
                return std::make_shared<ServiceDescribe>(std::move(_method_name),
                                                         std::move(_param_types),
                                                         _return_type,
                                                         std::move(_callback));
            }

        private:
            // RPC服务方法名,
            std::string _method_name;
            // RPC服务方法参数列表
            std::vector<ParamType> _param_types;
            // RPC服务方法返回值类型
            Type _return_type;
            // RPC服务方法回调函数
            ServerCallBack _callback;
        };

        class ServiceDescribeManager
        {
        public:
            using ptr = std::shared_ptr<ServiceDescribeManager>;
            void insertService(const ServiceDescribe::ptr &service)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _service_map.insert(std::make_pair(service->methodName(), service));
            }

            ServiceDescribe::ptr selectService(const std::string &name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _service_map.find(name);
                if (it == _service_map.end())
                {
                    ELOG("未找到对应的服务器 %s\n", name.c_str());
                    return ServiceDescribe::ptr();
                }
                return it->second;
            }

            void removeService(const std::string &name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _service_map.erase(name);
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, ServiceDescribe::ptr> _service_map;
        };

        class RpcRouter
        {
        public:
            using ptr = std::shared_ptr<RpcRouter>;
            RpcRouter()
                : _service_manager(std::make_shared<ServiceDescribeManager>())
            {
            }

            // 注册在dispatcher中的回调函数
            void OnRpcRequest(const BaseConnection::ptr &con, Json_rpc::RpcRequest::ptr &msg)
            {
                // 查找对应的服务
                auto service = _service_manager->selectService(msg->method());
                if (service == nullptr)
                {
                    response(con, msg, Json::Value(), RCode::RCODE_NOT_FOUND_RPCSERVICE);
                    return;
                }

                // 参数检查
                if (service->paramCheck(msg->params()) == false)
                {
                    response(con, msg, Json::Value(), RCode::RCODE_INVALID_PARAMS);
                    return;
                }

                // 调用服务
                Json::Value result;
                if (service->call(msg->params(), result) == false)
                {
                    response(con, msg, Json::Value(), RCode::RCODE_UNKNOWN_ERROR);
                    return;
                }

                // 返回结果
                response(con, msg, result, RCode::RCODE_SUCCESS);
            }

            void RegisterMethod(const ServiceDescribe::ptr &service)
            {
                _service_manager->insertService(service);
            }

        private:
            void response(const Json_rpc::BaseConnection::ptr &con, const Json_rpc::RpcRequest::ptr &msg,
                          const Json::Value &result, const Json_rpc::RCode &RCode)
            {
                // 创建一个回复消息
                auto response = Json_rpc::MessageFactory::FactoryCreate<Json_rpc::RpcResponse>();
                std::cout<<response->id().c_str()<<std::endl;
                response->setId(msg->id());
                response->setMType(Json_rpc::MType::RSP_RPC);
                response->setrcode(RCode);
                response->setresult(result);
                // 发送回复消息
                con->send(response);
            }

        private:
            ServiceDescribeManager::ptr _service_manager;
        };
    }

}