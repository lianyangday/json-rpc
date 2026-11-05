// #include "tool.hpp"
// using namespace Json_rpc;

// int main()
// {

//     // 序列化过程
//     Json::Value stduent;
//     stduent["name"] = "xiaoming";
//     stduent["age"] = 18;
//     stduent["socre"].append(77);
//     stduent["socre"].append(88);
//     std::string ss;
//     bool ret1 = serialize(stduent, ss);
//     if (ret1)
//         std::cout << "序列化的结果:\n"
//                   << ss.c_str() << std::endl;

//     // //反序列化过程
//     Json::Value root;
//     bool ret2 = unserialize("", root);
//     if (ret2)
//     {
//         std::cout << "序列化的结果:\n"
//                   << "name:" << root["name"].asString() << "\n"
//                   << "age:" << root["age"].asInt() << "\n"
//                   << "score:" << root["socre"][0].asInt() << "  " << root["socre"][1].asFloat() << "\n"
//                   << std::endl;
//     }

//     return 0;
// }

// int main()
// {
//     std::cout<<UUID::uuid()<<std::endl;
//     return 0;
// }

// message类的测试
// #include "message.hpp"

// int main()
// {

//     // RpcRequest测试
//     Json_rpc::RpcRequest::ptr tmp1 = Json_rpc::MessageFactory::FactoryCreate<Json_rpc::RpcRequest>();

//     // 设置数据
//     tmp1->setmethod("Add");
//     Json::Value val;
//     val["num1"] = 11;
//     val["num2"] = 22;
//     tmp1->setparams(val);

//     // 序列化
//     std::string str = tmp1->serialize();
//     std::cout << str << std::endl;

//     // check()
//     bool ret = tmp1->check();
//     if (ret == false)
//     {
//         return -1;
//     }

//     // 反序列化
//     // 在继承关系中，父类指针是可以指向子类对象的，但是子类指针是不允许指向父类对象的。
//     Json_rpc::BaseMessage::ptr tmp2 = Json_rpc::MessageFactory::FactoryCreate(MType::REQ_RPC);
//     // 如果在关系：父类指针指向子类对象，接着子类指针指向该父类，实际上等于子类指针指向子类对象
//     // Json_rpc::RpcRequest::ptr tmp3=tmp2;
//     // 上述写法是错误的，无法识别
//     // 需使用：std::dynamic_pointer_cast<>()   用于在运行时安全地将一个智能指针从基类类型转换为派生类类型
//     Json_rpc::RpcRequest::ptr tmp3 = std::dynamic_pointer_cast<Json_rpc::RpcRequest>(tmp2);
//     tmp3->unserialize(str);

//     // 输出反序列化结果
//     std::cout << tmp3->method() << std::endl;
//     Json::Value out = tmp3->params();

//     std::cout << out["num1"].asInt() << std::endl;
//     std::cout << out["num2"].asInt() << std::endl;

//     // TopicRequest测试
//     Json_rpc::TopicRequest::ptr tmp1 = Json_rpc::MessageFactory::FactoryCreate<Json_rpc::TopicRequest>();

//     // 设置数据
//     tmp1->setkey("news");
//     tmp1->setoptype(TOPIC_Optype::TOPIC_PUBLISH);
//     tmp1->setmsg("hello world");

//     // 序列化
//     std::string str = tmp1->serialize();
//     std::cout << str << std::endl;

//     // check()
//     bool ret = tmp1->check();
//     if (ret == false)
//     {
//         return -1;
//     }

//     // //反序列化
//     Json_rpc::BaseMessage::ptr tmp2 = Json_rpc::MessageFactory::FactoryCreate(MType::REQ_TOPIC);
//     Json_rpc::TopicRequest::ptr tmp3 = std::dynamic_pointer_cast<Json_rpc::TopicRequest>(tmp2);
//     tmp3->unserialize(str);

//     // 输出反序列化结果
//     std::cout << tmp3->key() << std::endl;
//     std::cout << (int)tmp3->optype() << std::endl;
//     std::cout << tmp3->msg() << std::endl;

//     // ServiceRequest测试
//     Json_rpc::ServiceRequest::ptr tmp1 = Json_rpc::MessageFactory::FactoryCreate<Json_rpc::ServiceRequest>();

//     // 设置数据
//     tmp1->setmethod("add");
//     tmp1->setoptype(SERVICE_Optype::SERVICE_DISCOVERY);
//     // Json_rpc::Address addr("127.0.0.1",9090);
//     // tmp1->sethost(addr);
//     // 序列化
//     std::string str = tmp1->serialize();
//     std::cout << str << std::endl;

//     // check()
//     bool ret = tmp1->check();
//     if (ret == false)
//     {
//         return -1;
//     }

//     // 反序列化
//     Json_rpc::BaseMessage::ptr tmp2 = Json_rpc::MessageFactory::FactoryCreate(MType::REQ_SERVICE);
//     Json_rpc::ServiceRequest::ptr tmp3 = std::dynamic_pointer_cast<Json_rpc::ServiceRequest>(tmp2);
//     tmp3->unserialize(str);

//     // 输出反序列化结果
//     std::cout << tmp3->method() << std::endl;
//     std::cout << (int)tmp3->optype() << std::endl;
//     Json_rpc::Address retaddr = tmp3->host();
//     std::cout << retaddr.first << ":" << retaddr.second << std::endl;
//     // RpcResponse测试
//     Json_rpc::RpcResponse::ptr tmp1 = Json_rpc::MessageFactory::FactoryCreate<Json_rpc::RpcResponse>();

//     // 设置数据
//     tmp1->setrcode(RCode::RCODE_SUCCESS);
//     tmp1->setresult("hello world");
//     // 序列化
//     std::string str = tmp1->serialize();
//     std::cout << str << std::endl;

//     // check()
//     bool ret = tmp1->check();
//     if (ret == false)
//     {
//         return -1;
//     }

//     // 反序列化
//     Json_rpc::BaseMessage::ptr tmp2 = Json_rpc::MessageFactory::FactoryCreate(MType::RSP_RPC);
//     Json_rpc::RpcResponse::ptr tmp3 = std::dynamic_pointer_cast<Json_rpc::RpcResponse>(tmp2);
//     tmp3->unserialize(str);

//     // 输出反序列化结果
//     std::cout << (int)tmp3->rcode() << std::endl;
//     std::cout << tmp3->result() << std::endl;

//     // TopicResponse测试
//     Json_rpc::TopicResponse::ptr tmp1 = Json_rpc::MessageFactory::FactoryCreate<Json_rpc::TopicResponse>();

//     // 设置数据
//     tmp1->setrcode(RCode::RCODE_SUCCESS);
//     // 序列化
//     std::string str = tmp1->serialize();
//     std::cout << str << std::endl;

//     // check()
//     bool ret = tmp1->check();
//     if (ret == false)
//     {
//         return -1;
//     }

//     // 反序列化
//     Json_rpc::BaseMessage::ptr tmp2 = Json_rpc::MessageFactory::FactoryCreate(MType::RSP_TOPIC);
//     Json_rpc::TopicResponse::ptr tmp3 = std::dynamic_pointer_cast<Json_rpc::TopicResponse>(tmp2);
//     tmp3->unserialize(str);

//     // 输出反序列化结果
//     std::cout << (int)tmp3->rcode() << std::endl;

//     // ServiceResponse测试
//     Json_rpc::ServiceResponse::ptr tmp1 = Json_rpc::MessageFactory::FactoryCreate<Json_rpc::ServiceResponse>();

//     // 设置数据
//     tmp1->setrcode(RCode::RCODE_SUCCESS);
//     tmp1->setoptype(SERVICE_Optype::SERVICE_DISCOVERY);
//     tmp1->setmethod("add");

//     std::vector<Json_rpc::Address> addrs;
//     addrs.push_back(Json_rpc::Address("127.0.0.1", 9090));
//     addrs.push_back(Json_rpc::Address("127.0.0.1", 9091));

//     tmp1->sethosts(addrs);

//     // 序列化
//     std::string str = tmp1->serialize();
//     std::cout << str << std::endl;

//     // check()
//     bool ret = tmp1->check();
//     if (ret == false)
//     {
//         return -1;
//     }

//     // 反序列化
//     Json_rpc::BaseMessage::ptr tmp2 = Json_rpc::MessageFactory::FactoryCreate(MType::RSP_SERVICE);
//     Json_rpc::ServiceResponse::ptr tmp3 = std::dynamic_pointer_cast<Json_rpc::ServiceResponse>(tmp2);
//     tmp3->unserialize(str);

//     // 输出反序列化结果
//     std::cout << (int)tmp3->rcode() << std::endl;
//     std::cout << (int)tmp3->optype() << std::endl;
//     std::vector<Json_rpc::Address> retaddrs;
//     retaddrs = tmp3->hosts();
//     for (auto &e : retaddrs)
//     {
//         std::cout << e.first << ":" << e.second << std::endl;
//     }

//     return 0;
// }
