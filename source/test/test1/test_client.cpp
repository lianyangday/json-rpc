#include <iostream>
#include "../../common/dispatcher.hpp"
#include "../../common/net.hpp"
#include "../../client/rpc_caller.hpp"

void result_cb(const Json::Value &result)
{
    std::cout << "任务已经在远端服务器完成" << std::endl;
}

int main()
{
    auto client = Json_rpc::ClientFactory::create("127.0.0.1", 9090);
    auto dispatcher = Json_rpc::DispatcherFactory::create();
    auto requestor = std::make_shared<Json_rpc::client::Requestor>();
    auto rpc_caller = std::make_shared<Json_rpc::client::RpcCaller>(requestor);
    auto cb = std::bind(&Json_rpc::client::Requestor::onResponse, requestor.get(), std::placeholders::_1, std::placeholders::_2);
    dispatcher->registerhandler<Json_rpc::ServiceResponse>(Json_rpc::MType::RSP_RPC, cb);
    auto onmessagecallback = bind(&Json_rpc::Dispatcher::onmessage, dispatcher.get(), std::placeholders::_1, std::placeholders::_2);
    client->setMessageCallback(onmessagecallback);
    client->connect();

    // 同步rpc请求
    Json::Value params;
    Json::Value result;
    params["num1"] = 11;
    params["num2"] = 22;
    // 向服务端发送rpc请求
    bool ret = rpc_caller->Call(client->connection(), "Add", params, result);
    if (ret)
    {
        ILOG("result: %s", result.asString().c_str());
    }

    // 异步rpc请求
    params["num1"] = 22;
    params["num2"] = 33;
    Json_rpc::client::RpcCaller::JsonAsyncResponse result_future;
    // 向服务端发送rpc请求
    ret = rpc_caller->Call(client->connection(), "Add", params, result_future);
    if (ret)
    {
        result = result_future.get();
        ILOG("result: %s", result.asString().c_str());
    }

    // 回调rpc请求
    params["num1"] = 50;
    params["num2"] = 50;
    // 向服务端发送rpc请求
    ret = rpc_caller->Call(client->connection(), "Add", params, result_cb);
    DLOG("--------");
    sleep(2);
    client->shutdown();
    std::cout << "连接断开！" << std::endl;

    return 0;
}