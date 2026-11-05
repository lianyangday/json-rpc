#include"../../server/rpc_server.hpp"
int main()
{
    auto _registry_server=std::make_shared<Json_rpc::server::RegistryServer>("127.0.0.1",9090);
    _registry_server->start();
    
    return 0;
}
