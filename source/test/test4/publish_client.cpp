#include"../../client/rpc_client.hpp"

int main()

{
    //实例化客户端对象
    auto client=std::make_shared<Json_rpc::client::TopicClient>("127.0.0.1",8080);
    //创建主题
    client->CreateTopic("sport");

    //发布消息
    while(1)
    {
    client->PublishTopic("sport","hello");
        sleep(1);
    }
    return 0;

}