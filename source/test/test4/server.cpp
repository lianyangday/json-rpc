#include"../../server/rpc_server.hpp"

int main()
{
    //实例化服务福安
    auto server=std::make_shared<Json_rpc::server::TopicServer>(8080);
    server->start();
    return 0;
}
