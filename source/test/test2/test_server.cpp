#include <iostream>
#include"../../server/rpc_server.hpp"


void Add(const Json::Value&req,Json::Value&rsp)
{
    int num1=req["num1"].asInt();
    int num2=req["num2"].asInt();
    rsp=num1+num2;
}

int main()
{
    //没有服务注册功能
    auto _server=std::make_shared<Json_rpc::server::RpcServer>(Json_rpc::Address("127.0.0.1",8080));
    //将rpc的回调函数注册带rpc_couter中
    std::unique_ptr<Json_rpc::server::ServiceDescribeFactory>des_factory(new Json_rpc::server::ServiceDescribeFactory);
    des_factory->set_menthod_name("Add");
    des_factory->set_param_types("num1",Json_rpc::server::Type::INTEGRAL);
    des_factory->set_param_types("num2",Json_rpc::server::Type::INTEGRAL);
    des_factory->set_return_type(Json_rpc::server::Type::INTEGRAL);
    des_factory->set_callback(Add);
    _server->RegisterMethod(des_factory->create());
    _server->start();
    return 0;
}