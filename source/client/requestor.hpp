#pragma once
#include "../common/message.hpp"
#include "../common/net.hpp"
#include <future>

namespace Json_rpc
{
    namespace client
    {
        class Requestor
        {
        public:
            using ptr = std::shared_ptr<Requestor>;
            using RequestCallBack = std::function<void(const BaseMessage::ptr &)>;
            using AsyncResponse = std::future<BaseMessage::ptr>;

            void onResponse(const BaseConnection::ptr &con, const BaseMessage::ptr &msg)
            {
                if(msg.get()==nullptr)
                {
                    ELOG("数据为空");
                }
                auto desc = getDescribe(msg->id());
                if (desc.get() == nullptr)
                {
                    ELOG("收到响应，但是没有 %s 的请求描述!", msg->id().c_str());
                    return;
                }
                if (desc->_rtype == RPC_Rtype::REQ_ASYNC)
                {
                    desc->_response.set_value(msg);
                }
                else if (desc->_rtype == RPC_Rtype::REQ_CALLBACK)
                {
                    if (desc->_callback != nullptr)
                    {
                        desc->_callback(msg);
                    }
                }
                else
                {
                    ELOG("收到响应，但是请求类型错误!");
                    return;
                }
                delDescribe(msg->id());
            }

            // 异步操作
            bool send(const BaseConnection::ptr &con, const BaseMessage::ptr &request, AsyncResponse &async_rsp)
            {
                RequestDescribe::ptr desc = newDescribe(request, RPC_Rtype::REQ_ASYNC);
                if (desc.get() == nullptr)
                {
                    ELOG("创建请求描述 %s 失败!", request->id().c_str());
                    return false;
                }
                con->send(request);
                async_rsp = desc->_response.get_future();
                return true;
            }

            // 回调操作
            bool send(const BaseConnection::ptr &con, const BaseMessage::ptr &request, const RequestCallBack &callback)
            {
                RequestDescribe::ptr desc = newDescribe(request, RPC_Rtype::REQ_CALLBACK, callback);
                if (desc.get() == nullptr)
                {
                    ELOG("创建请求描述 %s 失败!", request->id().c_str());
                    return false;
                }
                con->send(request);
                return true;
            }

            // 同步操作
            bool send(const BaseConnection::ptr &con, const BaseMessage::ptr &request, BaseMessage::ptr &response)
            {
                AsyncResponse async_rsp;
                bool ret = send(con, request, async_rsp);
                if (ret == false)
                {
                    return false;
                }
                response = async_rsp.get();
                return true;
            }

        private:
            // 这是Requestor类中用于描述每一个请求的内部结构体
            struct RequestDescribe
            {
            public:
                using ptr = std::shared_ptr<RequestDescribe>;
                BaseMessage::ptr _request;
                RPC_Rtype _rtype;
                std::promise<BaseMessage::ptr> _response;
                RequestCallBack _callback;
            };

        private:
            RequestDescribe::ptr newDescribe(const BaseMessage::ptr &request, RPC_Rtype rtype, const RequestCallBack &callback = RequestCallBack())
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto desc = std::make_shared<RequestDescribe>();
                desc->_request = request;
                desc->_rtype = rtype;
                if (rtype == RPC_Rtype::REQ_CALLBACK && callback != nullptr)
                {
                    desc->_callback = callback;
                }
                _request_map.insert(std::make_pair(request->id(), desc));
                return desc;
            }

            RequestDescribe::ptr getDescribe(const std::string &id)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _request_map.find(id);
                if (it == _request_map.end())
                {
                    return RequestDescribe::ptr();
                }
                return it->second;
            }

            void delDescribe(const std::string &id)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _request_map.erase(id);
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, RequestDescribe::ptr> _request_map;
        };
    }
}
