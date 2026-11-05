#include <iostream>
#include"../common/message.hpp"
#include"../common/dispatcher.hpp"
#include "../common/net.hpp"
#include"../server/rpc_router.hpp"

void Add(const Json::Value&req,Json::Value&rsp)
{
    int num1=req["num1"].asInt();
    int num2=req["num2"].asInt();
    rsp=num1+num2;
}

int main()
{
    auto server=Json_rpc::ServerFactory::create(9090);
    auto dispatcher=Json_rpc::DispatcherFactory::create();
    auto rpc_router=std::make_shared<Json_rpc::server::RpcRouter>();

    //将rpc的回调函数注册带rpc_couter中
    std::unique_ptr<Json_rpc::server::ServiceDescribeFactory>des_factory(new Json_rpc::server::ServiceDescribeFactory);
    des_factory->set_menthod_name("Add");
    des_factory->set_param_types("num1",Json_rpc::server::Type::INTEGRAL);
    des_factory->set_param_types("num2",Json_rpc::server::Type::INTEGRAL);
    des_factory->set_return_type(Json_rpc::server::Type::INTEGRAL);
    des_factory->set_callback(Add);
    rpc_router->RegisterMethod(des_factory->create());

    //将rpc_router服务注册到dispatcher中
    auto cb=std::bind(&Json_rpc::server::RpcRouter::OnRpcRequest,rpc_router.get(),std::placeholders::_1,std::placeholders::_2);
    dispatcher->registerhandler<Json_rpc::RpcRequest>(Json_rpc::MType::REQ_RPC,cb);

    //将dispatcher服务注册到server的消息回调函数中
    auto onmessagecallback=bind(&Json_rpc::Dispatcher::onmessage,dispatcher.get(),std::placeholders::_1,std::placeholders::_2);
    server->setMessageCallback(onmessagecallback);
    server->start();

    return 0;
}