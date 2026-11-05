#pragma once
#include "requestor.hpp"

namespace Json_rpc
{
    namespace client
    {
        class RpcCaller
        {
        public:
            using ptr = std::shared_ptr<RpcCaller>;
            using JsonAsyncResponse = std::future<Json::Value>;
            using JsonRequestCallBack = std::function<void(const Json::Value &)>;
            RpcCaller(const Requestor::ptr& requestor) : _requestor(requestor) {}

            // 同步操作
            bool Call(const BaseConnection::ptr &con, const std::string &method, const Json::Value &params, Json::Value &result)
            {
                RpcRequest::ptr req = MessageFactory::FactoryCreate<RpcRequest>();
                req->setId(UUID::uuid());
                req->setMType(MType::REQ_RPC);
                req->setmethod(method);
                req->setparams(params);
                BaseMessage::ptr rsp = MessageFactory::FactoryCreate<RpcResponse>();

                bool ret=_requestor->send(con, std::dynamic_pointer_cast<BaseMessage>(req), rsp);
                if(ret==false)
                {
                    ELOG("Rpc同步请求失败!");
                    return false;
                }

                RpcResponse::ptr ret_rsp = std::dynamic_pointer_cast<RpcResponse>(rsp);
                if (!ret_rsp)
                {
                    ELOG("Rpc响应,向下类型转换失败!");
                    return false;
                }

                // 检测返回消息是否正确
                if (ret_rsp->rcode() != RCode::RCODE_SUCCESS)
                {
                    ELOG("Rpc同步请求出错:%s", errReason(ret_rsp->rcode()).c_str());
                    return false;
                }

                result = ret_rsp->result();
                return true;
            }

            // 异步操作
            bool Call(const BaseConnection::ptr &con, const std::string &method, const Json::Value &params, JsonAsyncResponse &result)
            {
                RpcRequest::ptr req = MessageFactory::FactoryCreate<RpcRequest>();
                req->setId(UUID::uuid());
                req->setMType(MType::REQ_RPC);
                req->setmethod(method);
                req->setparams(params);
                BaseMessage::ptr rsp = MessageFactory::FactoryCreate<RpcResponse>();
                std::shared_ptr<std::promise<Json::Value>> Json_promise=std::make_shared<std::promise<Json::Value>>();
                result=Json_promise->get_future();
                Requestor::RequestCallBack cb=std::bind(&Json_rpc::client::RpcCaller::callback_one,this,Json_promise,std::placeholders::_1);
                bool ret=_requestor->send(con, std::dynamic_pointer_cast<BaseMessage>(req), cb);
                if(ret==false)
                {
                    ELOG("Rpc异步请求失败!");
                    return false;
                }
                return true;
            }

            // 回调操作
            bool Call(const BaseConnection::ptr &con, const std::string &method, const Json::Value &params, const JsonRequestCallBack &jsonreqcallback)
            {
                RpcRequest::ptr req = MessageFactory::FactoryCreate<RpcRequest>();
                req->setId(UUID::uuid());
                req->setMType(MType::REQ_RPC);
                req->setmethod(method);
                req->setparams(params);
                BaseMessage::ptr rsp = MessageFactory::FactoryCreate<RpcResponse>();
                Requestor::RequestCallBack cb=std::bind(&Json_rpc::client::RpcCaller::callback_two,this,jsonreqcallback,std::placeholders::_1);
                bool ret=_requestor->send(con, std::dynamic_pointer_cast<BaseMessage>(req), cb);
                if(ret==false)
                {
                    ELOG("Rpc异步请求失败!");
                    return false;
                }
                return true;
            }

        private:
            bool callback_one(std::shared_ptr<std::promise<Json::Value>> Json_promise, const BaseMessage::ptr &rsp)
            {
                RpcResponse::ptr ret_rsp = std::dynamic_pointer_cast<RpcResponse>(rsp);
                if (!ret_rsp)
                {
                    ELOG("Rpc响应,向下类型转换失败!");
                    return false;
                }

                // 检测返回消息是否正确
                if (ret_rsp->rcode() != RCode::RCODE_SUCCESS)
                {
                    ELOG("Rpc异步请求出错:%s", errReason(ret_rsp->rcode()).c_str());
                    return false;
                }

                Json_promise->set_value(ret_rsp->result());
                return true;
            }


            bool callback_two(const JsonRequestCallBack&jsonreqcallback,const BaseMessage::ptr&rsp)
            {
                RpcResponse::ptr ret_rsp = std::dynamic_pointer_cast<RpcResponse>(rsp);
                if (!ret_rsp)
                {
                    ELOG("Rpc响应,向下类型转换失败!");
                    return false;
                }

                // 检测返回消息是否正确
                if (ret_rsp->rcode() != RCode::RCODE_SUCCESS)
                {
                    ELOG("Rpc异步请求出错:%s", errReason(ret_rsp->rcode()).c_str());
                    return false;
                }
                jsonreqcallback(ret_rsp->result());
                return true;
            }

        private:
            Requestor::ptr _requestor;
        };
    }
}