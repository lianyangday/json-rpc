// 压测客户端：多线程 + 长连接，统计 QPS 与延迟分位数
//
// 用法: ./bench [线程数] [每线程请求数] [ip] [port]
// 示例: ./bench 1 20000 127.0.0.1 8080
//
// 注意: 每个线程持有独立连接，避免共享 Requestor 的响应表带来锁竞争。

#include <chrono>
#include <vector>
#include <thread>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "client/rpc_client.hpp"

using Clock = std::chrono::steady_clock;

static double percentile(std::vector<double> &v, double p)
{
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t idx = (size_t)(p * (double)(v.size() - 1));
    return v[idx];
}

int main(int argc, char **argv)
{
    int threads = (argc > 1) ? std::atoi(argv[1]) : 1;
    int per     = (argc > 2) ? std::atoi(argv[2]) : 20000;
    std::string ip = (argc > 3) ? argv[3] : "127.0.0.1";
    int port    = (argc > 4) ? std::atoi(argv[4]) : 8080;

    if (threads <= 0 || per <= 0) {
        std::fprintf(stderr, "参数非法\n");
        return 1;
    }

    std::vector<std::vector<double>> lat(threads);
    std::vector<int> errs(threads, 0);
    std::vector<std::thread> ts;

    auto t_begin = Clock::now();

    for (int i = 0; i < threads; ++i) {
        ts.emplace_back([&, i]() {
            Json_rpc::client::RpcClient cli(false, ip, port);

            Json::Value params;
            params["num1"] = 1;
            params["num2"] = 2;

            // 预热：排除建连、首次序列化等一次性开销
            for (int k = 0; k < 500; ++k) {
                Json::Value rsp;
                cli.Call("Add", params, rsp);
            }

            // 等连接就绪（muduo 建连是异步的）
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            lat[i].reserve(per);
            for (int k = 0; k < per; ++k) {
                Json::Value rsp;
                auto t0 = Clock::now();
                cli.Call("Add", params, rsp);
                auto t1 = Clock::now();

                if (rsp.isNull()) {
                    errs[i]++;
                    continue;
                }
                lat[i].push_back(
                    std::chrono::duration<double, std::micro>(t1 - t0).count());
            }
        });
    }

    for (auto &t : ts) t.join();
    auto t_end = Clock::now();

    std::vector<double> all;
    int total_err = 0;
    for (int i = 0; i < threads; ++i) {
        all.insert(all.end(), lat[i].begin(), lat[i].end());
        total_err += errs[i];
    }

    double sec = std::chrono::duration<double>(t_end - t_begin).count();
    if (sec <= 0) sec = 1e-9;

    std::printf("threads=%d  ok=%zu  err=%d  time=%.2fs  QPS=%.0f\n",
                threads, all.size(), total_err, sec, all.size() / sec);
    std::printf("P50=%.0f  P90=%.0f  P99=%.0f  max=%.0f  (us)\n",
                percentile(all, 0.50), percentile(all, 0.90),
                percentile(all, 0.99), percentile(all, 1.00));
    return 0;
}
