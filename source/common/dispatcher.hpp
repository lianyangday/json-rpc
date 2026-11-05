#pragma once
#include "net.hpp"
#include "message.hpp"
#include "abstract.hpp"
#include <unordered_map>
#include <mutex>
namespace Json_rpc
{
    class Callback
    {
    public:
        using ptr = std::shared_ptr<Callback>;
        virtual void onmessage(const BaseConnection::ptr &con, BaseMessage::ptr &msg) = 0;
    };

    template <typename T>
    class CallbackT : public Callback
    {
    public:
        using ptr = std::shared_ptr<CallbackT<T>>;
        using CallbackFunc = std::function<void(const Json_rpc::BaseConnection::ptr&, typename T::ptr&)>;
        CallbackT(const CallbackFunc &handler) : _handler(handler) {}
        virtual void onmessage(const BaseConnection::ptr &con, BaseMessage::ptr &msg) override
        {
            auto _msg = std::dynamic_pointer_cast<T>(msg);
            _handler(con, _msg);
        }

    private:
        CallbackFunc _handler;
    };

    class Dispatcher
    {
    public:
        using ptr = std::shared_ptr<Dispatcher>;
        // 注册函数
        template <typename T>
        void registerhandler(MType type, const typename CallbackT<T>::CallbackFunc &handler)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto cb=std::make_shared<CallbackT<T>>(handler);
            _handlers.insert(std::make_pair(type, cb));
        }

        void onmessage(const BaseConnection::ptr &con, BaseMessage::ptr &msg)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _handlers.find(msg->mtype());
            if (it != _handlers.end())
            {
                it->second->onmessage(con, msg);
                return;
            }
            // 出现未知的消息类型
            ELOG("未知的消息类型!");
            con->shutdown();
        }

    private:
        // 多线程使用共享资源，共享资源需要使用锁进行保护
        std::mutex _mutex;

        // 实现Mtype可以映射不同类型的函数
        std::unordered_map<MType, Callback::ptr> _handlers;
    };

    class DispatcherFactory
    {
    public:
        template <typename... Args>
        static Dispatcher::ptr create(Args &&...args)
        {
            return std::make_shared<Dispatcher>(std::forward<Args>(args)...);
        }
    };
}