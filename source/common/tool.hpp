#pragma once
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>
#include <jsoncpp/json/json.h>
#include <random>
#include <iomanip>
#include <atomic>

namespace Json_rpc
{

    // 日志系统
#define LDBG 0
#define LINF 1
#define LERR 2

#define LDEFAULT LDBG

#define LOG(level, format, ...)                                                                      \
    {                                                                                                \
        if (level >= LDEFAULT)                                                                       \
        {                                                                                            \
            time_t t = time(NULL);                                                                   \
            struct tm *it = localtime(&t);                                                           \
            char time_tmp[32] = {0};                                                                 \
            strftime(time_tmp, 31, "%m-%d %T", it);                                                  \
            fprintf(stdout, "[%s][%s:%d]" format "\n", time_tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
        }                                                                                            \
    }

#define DLOG(format, ...) LOG(LDBG, format, ##__VA_ARGS__)
#define ILOG(format, ...) LOG(LINF, format, ##__VA_ARGS__)
#define ELOG(format, ...) LOG(LERR, format, ##__VA_ARGS__)

    // json序列化和反序列化
    class JSONTOOL
    {
        public:
        static bool serialize(const Json::Value &root, std::string &str)
        {
            Json::StreamWriterBuilder swb;
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

            std::stringstream ss;
            int ret = sw->write(root, &ss);
            if (ret != 0)
            {
                ELOG("%s", "Serialize failed !");
                return false;
            }
            str = ss.str();
            return true;
        }

        static bool unserialize(const std::string &str, Json::Value &root)
        {
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

            std::stringstream ss;
            bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, nullptr);
            if (!ret)
            {
                ELOG("%s", "Unserialize failed !");
                return false;
            }
            return true;
        }
    };

    // UUID的生成
    //: 32位的十六机制字符
    //: 格式为8-4-4-4-12
    class UUID
    {
        public:
        static std::string uuid()
        {
            // 1.机器随机数对象
            std::random_device rd;

            // 2.机器数作为随机种子给伪随机对象
            std::mt19937 generator(rd());

            // 3.确定随机数的范围        ::一个16进制的字符需要四个比特，这里打算两个字符一起生成，也就是八个比特:00000000-11111111。
            std::uniform_int_distribution<int> distribution(0, 256);

            // 4.八个随机数
            std::stringstream ss;
            for (int i = 0; i < 8; i++)
            {
                if (i == 4 || i == 6)
                    ss << '-';

                ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
            }

            ss << '-';

            // 5.后八位为序列号，多线程公有的数据，使用atomic原子锁
            static std::atomic<size_t> sep(1); // 计数使用
            size_t count = sep.fetch_add(1);

            // 6.将计数转为16进制字符串  00 00 00 00 00 00 00 01
            for (int i = 7; i >= 0; i--)
            {
                if (i == 5)
                    ss << '-';
                ss << std::setw(2) << std::setfill('0') << std::hex << ((count >> i * 8) & 0xFF);
            }

            return ss.str();
        }
    };

}
