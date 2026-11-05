#pragma once

#include "abstract.hpp"
#include "fields.hpp"
#include "tool.hpp"
#include <vector>

namespace Json_rpc
{

    using Address = std::pair<std::string, int>;

    class JsonMessage : public BaseMessage
    {
    public:
        using ptr = std::shared_ptr<JsonMessage>;

        virtual std::string serialize() override
        {
            std::string ret_body;
            bool ret = JSONTOOL::serialize(_body, ret_body);
            if (ret == false)
            {
                return std::string();
            }
            return ret_body;
        }

        virtual bool unserialize(const std::string &body) override
        {
            bool ret = JSONTOOL::unserialize(body, _body);
            return ret;
        }

    protected:
        Json::Value _body;
    };

    class JsonRequest : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonRequest>;
    };

    class RpcRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<RpcRequest>;
        virtual bool check() override
        {
            // 判断方法和参数是否存在，类型是否错误
            if (_body[KEY_METHOD].isNull() == true || _body[KEY_METHOD].isString() == false)
            {
                ELOG("RPC请求中没有方法名称或者方法名称的类型错误!");
                return false;
            }

            if (_body[KEY_PRC_PARAMS].isNull() == true || _body[KEY_PRC_PARAMS].isObject() == false)
            {
                ELOG("RPC请求中没有参数信息或者参数信息的类型错误!");
                return false;
            }

            return true;
        }

        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        void setmethod(const std::string &method_name)
        {
            _body[KEY_METHOD] = method_name;
        }

        Json::Value params()
        {
            return _body[KEY_PRC_PARAMS];
        }

        void setparams(const Json::Value &rpc_params)
        {
            _body[KEY_PRC_PARAMS] = rpc_params;
        }
    };

    class TopicRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<TopicRequest>;

        virtual bool check() override
        {
            if (_body[KEY_TOPIC_KEY].isNull() == true || _body[KEY_TOPIC_KEY].isString() == false)
            {
                ELOG("TOPIC请求中没有主题名称或者主题名称的类型错误!");
                return false;
            }

            if (_body[KEY_OPTYPE].isNull() == true || _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("TOPIC请求中没有操作数或者操作数的类型错误!");
                return false;
            }

            if (_body[KEY_OPTYPE].asInt() == (int)(TOPIC_Optype::TOPIC_PUBLISH) && (_body[KEY_TOPIC_MSY].isNull() == true || _body[KEY_TOPIC_MSY].isString() == false))
            {
                ELOG("TOPIC请求中没有发布信息或者发布信息的类型错误!");
                return false;
            }

            return true;
        }

        std::string key()
        {
            return _body[KEY_TOPIC_KEY].asString();
        }

        void setkey(const std::string &topic_key)
        {
            _body[KEY_TOPIC_KEY] = topic_key;
        }

        TOPIC_Optype optype()
        {
            return (TOPIC_Optype)_body[KEY_OPTYPE].asInt();
        }

        void setoptype(const TOPIC_Optype &topic_optype)
        {
            _body[KEY_OPTYPE] = (int)topic_optype;
        }

        std::string msg()
        {
            return _body[KEY_TOPIC_MSY].asString();
        }

        void setmsg(const std::string &topic_msg)
        {
            _body[KEY_TOPIC_MSY] = topic_msg;
        }
    };

    class ServiceRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<ServiceRequest>;

        virtual bool check() override
        {
            if (_body[KEY_METHOD].isNull() == true || _body[KEY_METHOD].isString() == false)
            {
                ELOG("SERVICE请求中没有方法名称或者方法名称的类型错误!");
                return false;
            }

            if (_body[KEY_OPTYPE].isNull() == true || _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("SERVICE请求中没有操作数或者操作数的类型错误!");
                return false;
            }

            if (_body[KEY_OPTYPE].asInt() != (int)(SERVICE_Optype::SERVICE_DISCOVERY) && (_body[KEY_HOST].isNull() == true || _body[KEY_HOST].isObject() == false ||
                                                                                          _body[KEY_HOST][KEY_HOST_IP].isNull() == true || _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
                                                                                          _body[KEY_HOST][KEY_HOST_PORT].isNull() == true || _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false))
            {
                ELOG("SERVICE请求中主机地址错误!");
                return false;
            }

            return true;
        }

        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        void setmethod(const std::string &method_name)
        {
            _body[KEY_METHOD] = method_name;
        }

        SERVICE_Optype optype()
        {
            return (SERVICE_Optype)_body[KEY_OPTYPE].asInt();
        }

        void setoptype(const SERVICE_Optype &service_optype)
        {
            _body[KEY_OPTYPE] = (int)service_optype;
        }

        Address host()
        {
            Address ret;
            ret.first = _body[KEY_HOST][KEY_HOST_IP].asString();
            ret.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
            return ret;
        }

        void sethost(const Address &host)
        {
            Json::Value _host;
            _host[KEY_HOST_IP] = host.first;
            _host[KEY_HOST_PORT] = host.second;
            _body[KEY_HOST] = _host;

            // _body[KEY_HOST][KEY_HOST_IP] = host.first;
            // _body[KEY_HOST][KEY_HOST_PORT] = host.second;
        }
    };

    class JsonResponse : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonResponse>;

        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("RESPONSE回应中没有相应码或者响应码的类型错误!");
                return false;
            }
            return true;
        }

        RCode rcode()
        {
            return (RCode)_body[KEY_RCODE].asInt();
        }

        void setrcode(const RCode &rcode)
        {
            _body[KEY_RCODE] = (int)rcode;
        }
    };

    class RpcResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<RpcResponse>;

        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("RPCRESPONSE回应中没有相应码或者响应码的类型错误!");
                return false;
            }

            if (_body[KEY_RESULT].isNull() == true)
            {
                ELOG("RPCRESPONSE回应中没有调用结果或者调用结果的类型错误!");
                return false;
            }

            return true;
        }

        Json::Value result()
        {
            return _body[KEY_RESULT];
        }

        void setresult(const Json::Value &result)
        {
            _body[KEY_RESULT] = result;
        }
    };

    class TopicResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<TopicResponse>;
        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("TOPICRESPONSE回应中没有相应码或者响应码的类型错误!");
                return false;
            }
            return true;
        }
    };

    class ServiceResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<ServiceResponse>;

        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("SERVICERESPONSE回应中没有相应码或者响应码的类型错误!");
                return false;
            }

            if (_body[KEY_OPTYPE].isNull() == true || _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("SERVICERESPONSE回应中没有操作数或者操作数的类型错误!");
                return false;
            }

            if ((_body[KEY_OPTYPE] == (int)SERVICE_Optype::SERVICE_DISCOVERY) && (_body[KEY_METHOD].isNull() == true || _body[KEY_METHOD].isString() == false ||
                                                                                  _body[KEY_HOST].isNull() == true || _body[KEY_HOST].isArray() == false))
            {
                std::cout<<_body[KEY_METHOD].isNull()<<_body[KEY_METHOD].isString()<<_body[KEY_HOST].isNull()<<_body[KEY_HOST].isArray()<<std::endl;
                ELOG("SERVICERESPONSE服务发现回应中相应信息字段错误!");
                return false;
            }

            return true;
        }

        SERVICE_Optype optype()
        {
            return (SERVICE_Optype)_body[KEY_OPTYPE].asInt();
        }

        void setoptype(const SERVICE_Optype &service_optype)
        {
            _body[KEY_OPTYPE] = (int)service_optype;
        }

        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        void setmethod(const std::string &method_name)
        {
            _body[KEY_METHOD] = method_name;
        }

        std::vector<Address> hosts()
        {
            std::vector<Address> addres;
            int n = _body[KEY_HOST].size();
            for (int i = 0; i < n; i++)
            {
                Address addre;
                addre.first = _body[KEY_HOST][i][KEY_HOST_IP].asString();
                addre.second = _body[KEY_HOST][i][KEY_HOST_PORT].asInt();
                addres.push_back(addre);
            }
            return addres;
        }

        void sethosts(const std::vector<Address> &addres)
        {
            int n = addres.size();
            for (auto &addre : addres)
            {
                Json::Value val;
                val[KEY_HOST_IP] = addre.first;
                val[KEY_HOST_PORT] = addre.second;
                _body[KEY_HOST].append(val);
            }
        }
    };

    // 创建一个生产消息对象的工厂类

    class MessageFactory
    {
    public:
        // c++不能返回抽象类本身，但是可以返回抽象类的指针，只要之后指针所指向的是实例化的类就可以
        static BaseMessage::ptr FactoryCreate(MType type)
        {
            switch (type)
            {
            case MType::REQ_RPC:
                return std::make_shared<RpcRequest>();
            case MType::RSP_RPC:
                return std::make_shared<RpcResponse>();
            case MType::REQ_TOPIC:
                return std::make_shared<TopicRequest>();
            case MType::RSP_TOPIC:
                return std::make_shared<TopicResponse>();
            case MType::REQ_SERVICE:
                return std::make_shared<ServiceRequest>();
            case MType::RSP_SERVICE:
                return std::make_shared<ServiceResponse>();
            default:
                // 代码错误位置：
                //  return std::make_shared<BaseMessage>();
                // 错误原因：make_shared函数，需要先初始化对象，再将shared_ptr指针指向该对象
                // 续：但是BaseMessage对象是一个虚类，不能被实例化

                // 代码修改：share_ptr<BaseMessage>()  相当于只能指针指指向为空
                return BaseMessage::ptr();
            }
        }

        template <class T, class... Args>
        static std::shared_ptr<T> FactoryCreate(Args &&...args)
        {
            return std::make_shared<T>(std::forward(args)...);
        }
    };

}
