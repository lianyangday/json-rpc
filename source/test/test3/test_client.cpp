#include "../../client/rpc_client.hpp"
void result_cb(const Json::Value &result)
{
    std::cout << "任务已经在远端服务器完成" << std::endl;
}

int main()
{
    auto _client=std::make_shared<Json_rpc::client::RpcClient>(true,"127.0.0.1",9090);

    // 同步rpc请求
    Json::Value params;
    Json::Value result;
    params["num1"] = 11;
    params["num2"] = 22;
    // 向服务端发送rpc请求
    bool ret = _client->Call( "Add", params, result);
    if (ret)
    {
        ILOG("result: %s", result.asString().c_str());
    }

    // 异步rpc请求
    params["num1"] = 22;
    params["num2"] = 33;
    Json_rpc::client::RpcCaller::JsonAsyncResponse result_future;
    // 向服务端发送rpc请求
    ret = _client->Call("Add", params, result_future);
    if (ret)
    {
        result = result_future.get();
        ILOG("result: %s", result.asString().c_str());
    }

    // 回调rpc请求
    params["num1"] = 50;
    params["num2"] = 50;
    // 向服务端发送rpc请求
    ret = _client->Call("Add", params, result_cb);
    DLOG("--------");
    sleep(2);
    std::cout << "连接断开！" << std::endl;

    return 0;
}