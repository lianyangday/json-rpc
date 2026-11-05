#include "requestor.hpp"

namespace Json_rpc
{
    namespace client
    {
        class TopicManager
        {
        public:
            using ptr = std::shared_ptr<TopicManager>;
            using SubCallback = std::function<void(const std::string &key, const std::string &msg)>;
            TopicManager(const Requestor::ptr &requestor) : _requestor(requestor) {}
            // 主题创建
            bool CreateTopic(const BaseConnection::ptr &conn, const std::string &key)
            {
                return CommonRequest(conn, key, TOPIC_Optype::TOPIC_CREATE);
            }
            // 主题删除
            bool RemoveTopic(const BaseConnection::ptr &conn, const std::string &key)
            {
                return CommonRequest(conn, key, TOPIC_Optype::TOPIC_REMOVE);
            }
            // 主题订阅:需要添加对应的回调函数
            bool SubcribeTopic(const BaseConnection::ptr &conn, const std::string &key, const SubCallback &sub_cb)
            {
                // 为了保证回调函数的同步性
                // 方案一:使用mutex,将这个操作包含其中
                // 方案二:提前添加,topic请求失败就,进行删除
                AddSubCallback(key, sub_cb);
                bool ret = CommonRequest(conn, key, TOPIC_Optype::TOPIC_SUBSCRIBE);
                if (ret == false)
                {
                    DelSubCallback(key);
                    return false;
                }
                return true;
            }
            // 主题取消订阅
            bool CancelTopic(const BaseConnection::ptr &conn, const std::string &key)
            {
                DelSubCallback(key);
                return CommonRequest(conn, key, TOPIC_Optype::TOPIC_SUBSCRIBE);
            }
            // 主题消息的发布
            bool PublishTopic(const BaseConnection::ptr &conn, const std::string &key, const std::string &msg)
            {
                return CommonRequest(conn, key, TOPIC_Optype::TOPIC_PUBLISH, msg);
            }

            // 注册到dispatcher中的topicrequest的回调函数
            void OnPublish(const BaseConnection::ptr &con, const TopicRequest::ptr &msg)
            {
                // 1.判断消息的操作类型
                TOPIC_Optype optype = msg->optype();
                std::string key = msg->key();
                std::string message=msg->msg();
                if (optype != TOPIC_Optype::TOPIC_PUBLISH)
                {
                    ELOG("收到错误的主题操作类型!");
                    return;
                }

                // 查找对应的函数
                SubCallback sub_cb = GetSubCallback(key);
                if (!sub_cb)
                {
                    ELOG("收到 %s 主题消息,但是没有对应的回调函数!",key.c_str());
                    return;
                }

                //调用对应的函数
                sub_cb(key,message);
            }

        private:
            bool CommonRequest(const BaseConnection::ptr &conn, const std::string &key, TOPIC_Optype optype, const std::string &msg = "")
            {
                TopicRequest::ptr req = MessageFactory::FactoryCreate<TopicRequest>();
                req->setId(UUID::uuid());
                req->setkey(key);
                req->setMType(MType::REQ_TOPIC);
                req->setoptype(optype);
                if (optype == TOPIC_Optype::TOPIC_PUBLISH)
                {
                    req->setmsg(msg);
                }

                // 异步方式:消息的接受
                BaseMessage::ptr rsp;

                bool ret = _requestor->send(conn, req, rsp);
                if (ret == false)
                {
                    ELOG("Topic主题操作创建失败!");
                    return false;
                }

                TopicResponse::ptr ret_rsp = std::dynamic_pointer_cast<TopicResponse>(rsp);
                if (!ret_rsp)
                {
                    ELOG("Topic主题操作响应,向下类型转换失败!");
                    return false;
                }

                // 检测返回消息是否正确
                if (ret_rsp->rcode() != RCode::RCODE_SUCCESS)
                {
                    ELOG("Topic主题操作出错:%s", errReason(ret_rsp->rcode()).c_str());
                    return false;
                }
                return true;
            }

            SubCallback GetSubCallback(const std::string &key)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _topic_callbacks.find(key);
                if (it == _topic_callbacks.end())
                {
                    return SubCallback();
                }
                return it->second;
            }

            void AddSubCallback(const std::string &key, const SubCallback &sub_cb)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _topic_callbacks.insert(std::make_pair(key, sub_cb));
            }

            void DelSubCallback(const std::string &key)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _topic_callbacks.erase(key);
            }

        private:
            std::mutex _mutex;
            Requestor::ptr _requestor;
            std::unordered_map<std::string, SubCallback> _topic_callbacks;
        };
    }
}