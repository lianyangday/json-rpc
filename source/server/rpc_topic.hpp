#pragma once
#include "../common/net.hpp"
#include "../common/message.hpp"
#include <unordered_set>
#include <unordered_map>

namespace Json_rpc
{
    namespace server
    {
        class TopicManager
        {
        public:
            using ptr = std::shared_ptr<TopicManager>;
            void OnTopicRequest(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg)
            {
                // 主题的创建
                // 主题的删除
                // 主题的订阅
                // 主题的取消订阅
                // 主题消息的发布
                TOPIC_Optype optype = msg->optype();
                bool ret=true;
                switch (optype)
                {
                case TOPIC_Optype::TOPIC_CREATE:
                    TopicCreate(con, msg);
                    break;
                case TOPIC_Optype::TOPIC_REMOVE:
                    TopicRemove(con, msg);
                    break;
                case TOPIC_Optype::TOPIC_SUBSCRIBE:
                    ret = TopicSubscribers(con, msg);
                    break;
                case TOPIC_Optype::TOPIC_CANCEL:
                    TopicCancel(con, msg);
                    break;
                case TOPIC_Optype::TOPIC_PUBLISH:
                    ret = TopicPublish(con, msg);
                    break;
                default:
                    return RerrorResponse(con, msg, RCode::RCODE_INVALID_OPTYPE);
                    break;
                }
                if (ret == false)
                    return RerrorResponse(con, msg, RCode::RCODE_NOT_FOUND_TOPIC);
                else
                    return SuccessResponse(con, msg);
            }
            void OnShutdown(const BaseConnection::ptr &con)
            {
                std::vector<Topic::ptr> topics;
                Subscriber::ptr subscriber;
                // 1.判断连接着是主题发布者还是主题订阅者
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _subscribers.find(con);
                    if (it == _subscribers.end())
                    {
                        // 主题发布者,不做任何处理
                        return;
                    }

                    // 主题订阅者:将该订阅者从订阅者表中和主题表中删除
                    subscriber = it->second;
                    for (auto &topic_name : subscriber->_topic_names)
                    {
                        auto topic_it = _topics.find(topic_name);
                        if (topic_it == _topics.end())
                        {
                            continue;
                        }
                        topics.push_back(topic_it->second);
                    }

                    _subscribers.erase(it);
                }
                // 主题删除对应的订阅者
                for (auto &topic : topics)
                {
                    topic->RemoveSubscriber(subscriber);
                }
            }

        private:
            void RerrorResponse(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg, RCode rcode)
            {
                TopicResponse::ptr sec_rsp = MessageFactory::FactoryCreate<TopicResponse>();
                sec_rsp->setId(msg->id());
                sec_rsp->setMType(MType::RSP_TOPIC);
                sec_rsp->setrcode(rcode);
                con->send(sec_rsp);
            }

            void SuccessResponse(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg)
            {
                TopicResponse::ptr sec_rsp = MessageFactory::FactoryCreate<TopicResponse>();
                sec_rsp->setId(msg->id());
                sec_rsp->setMType(MType::RSP_TOPIC);
                sec_rsp->setrcode(RCode::RCODE_SUCCESS);
                con->send(sec_rsp);
            }
            // 主题的创建
            void TopicCreate(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg)
            {
                std::string topic_name = msg->key();
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _topics.find(topic_name);
                if (it != _topics.end())
                {
                    return;
                }
                Topic::ptr topic = std::make_shared<Topic>(topic_name);
                _topics.insert(std::make_pair(topic_name, topic));
            }

            // 主题的删除
            void TopicRemove(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg)
            {
                std::string topic_name = msg->key();
                Topic::ptr topic;
                std::unordered_set<Subscriber::ptr> subscribers;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _topics.find(topic_name);
                    if (it == _topics.end())
                    {
                        return;
                    }

                    topic = it->second;
                    subscribers = topic->_subscribers;
                    _topics.erase(it);
                }

                // 每个锁只保护自己对应的数据类型
                for (auto &subscriber : subscribers)
                {
                    subscriber->RemoveTopic(topic_name);
                }
            }

            // 主题的订阅
            bool TopicSubscribers(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg)
            {
                std::string topic_name = msg->key();
                Topic::ptr topic;
                Subscriber::ptr subscriber;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(topic_name);
                    // 找不到主题是订阅错误
                    if (topic_it == _topics.end())
                    {
                        return false;
                    }

                    topic = topic_it->second;

                    auto sub_it = _subscribers.find(con);
                    if (sub_it != _subscribers.end())
                    {
                        subscriber = sub_it->second;
                    }
                    else
                    {
                        subscriber = std::make_shared<Subscriber>(con);
                        _subscribers.insert(std::make_pair(con, subscriber));
                    }
                    topic->AppendSubscriber(subscriber);
                    subscriber->AppendTopic(topic_name);
                    return true;
                }
            }

            // 主题的取消订阅
            void TopicCancel(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg)
            {
                std::string topic_name = msg->key();
                Topic::ptr topic;
                Subscriber::ptr subscriber;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(topic_name);
                    // 找不到主题是订阅错误
                    if (topic_it != _topics.end())
                    {
                        topic = topic_it->second;
                    }

                    auto sub_it = _subscribers.find(con);
                    if (sub_it != _subscribers.end())
                    {
                        subscriber = sub_it->second;
                    }
                }
                if (subscriber)
                    subscriber->RemoveTopic(topic_name);
                if (topic && subscriber)
                    topic->RemoveSubscriber(subscriber);
            }

            bool TopicPublish(const BaseConnection::ptr &con, Json_rpc::TopicRequest::ptr &msg)
            {
                std::string topic_name = msg->key();
                Topic::ptr topic;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(topic_name);
                    // 找不到主题是订阅错误
                    if (topic_it == _topics.end())
                    {
                        return false;
                    }
                    topic = topic_it->second;
                }
                topic->PutMessage(msg);
                return true;
            }

        private:
            struct Subscriber
            {
                using ptr = std::shared_ptr<Subscriber>;
                std::mutex _mutex;
                BaseConnection::ptr _conn;
                std::unordered_set<std::string> _topic_names;

                Subscriber(const BaseConnection::ptr &conn) : _conn(conn) {}
                void AppendTopic(const std::string &topic_name)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _topic_names.insert(topic_name);
                }
                void RemoveTopic(const std::string &topic_name)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _topic_names.erase(topic_name);
                }
            };

            struct Topic
            {
                using ptr = std::shared_ptr<Topic>;
                std::mutex _mutex;
                std::string _topic_name;
                std::unordered_set<Subscriber::ptr> _subscribers;

                Topic(const std::string &topic_name) : _topic_name(topic_name) {}
                void AppendSubscriber(const Subscriber::ptr &subscriber)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _subscribers.insert(subscriber);
                }
                void RemoveSubscriber(const Subscriber::ptr &subscriber)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _subscribers.erase(subscriber);
                }

                // 进行主题消息的转发
                void PutMessage(const BaseMessage::ptr &msg)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    for (auto &it : _subscribers)
                    {
                        it->_conn->send(msg);
                    }
                }
            };

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, Topic::ptr> _topics;
            std::unordered_map<BaseConnection::ptr, Subscriber::ptr> _subscribers;
        };
    }
}