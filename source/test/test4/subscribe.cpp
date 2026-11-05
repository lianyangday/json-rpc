#include"../../client/rpc_client.hpp"
#include<iostream>
void sport_cb(const std::string &key, const std::string &msg)
{
    std::cout<<"收到"<<key.c_str()<<"主题的消息: "<<msg.c_str()<<std::endl;
}
int main()
{
    //实例化客户端对象
    auto client=std::make_shared<Json_rpc::client::TopicClient>("127.0.0.1",8080);
    //订阅主题
    client->SubcribeTopic("sport",sport_cb);
    sleep(10);
    return 0;

}