# Json-RPC 远程调用框架

基于 C++11 + muduo 网络库实现的轻量 RPC 框架，支持 **RPC 远程调用**、**服务注册与发现**、**主题发布订阅** 三种通信模式。

## 特性

- **协议抽象**：消息类型、通信类型、网络协议三层抽象，底层传输可替换
- **多消息类型统一处理**：`BaseMessage` 派生 RPC / Service / Topic 三类消息，Dispatcher 按类型自动分发
- **同步 / 异步 / 回调**：基于 `std::future` 统一三种调用模式
- **服务注册与发现**：服务端注册方法到注册中心，客户端按方法名发现服务地址
- **主题发布订阅**：支持多订阅者 fanout

## 目录结构

```
├── source/
│   ├── common/        # 消息基类、Dispatcher、字段定义、网络工具
│   ├── client/        # RpcClient、服务发现客户端、请求者、Topic 客户端
│   ├── server/        # RpcServer、方法注册与路由、注册中心服务端
│   └── test/          # 四个阶段的测试用例（test1~test4）
├── muduo-master/      # 依赖的 muduo 源码（构建时自动编译）
├── bench.cpp          # 压测客户端
└── build.sh           # 一键构建脚本
```

## 依赖

```bash
sudo apt install -y build-essential cmake libjsoncpp-dev libboost-dev
```

## 构建

```bash
chmod +x build.sh
./build.sh
```

脚本会先编译 `muduo-master` 并安装到 `build/release-install-cpp11/`，再编译各测试用例与压测程序。

## 运行

**基础 RPC 调用（test1 / test2）**

```bash
./source/test/test2/server      # 默认监听 127.0.0.1:8080
./source/test/test2/client
```

**服务注册与发现（test3）**

```bash
./source/test/test3/registry    # 注册中心
./source/test/test3/server      # 服务提供者，向注册中心注册
./source/test/test3/client      # 客户端，按方法名发现服务
```

**发布订阅（test4）**

```bash
./source/test/test4/server
./source/test/test4/subscribe_client
./source/test/test4/publish_client
```

## 压测

```bash
# 并发数 线程数 / 每线程请求数 / 目标 ip / 端口
./bench 1 20000 127.0.0.1 8080
```

输出示例：

```
threads=1 reqs=20000 time=1.83s QPS=10929
P50=85 P90=120 P99=210 max=1800 (us)
```

> 单机压测时建议把服务端与客户端绑定到不同 CPU 核，减少调度干扰：
> `taskset -c 0 ./server` 与 `taskset -c 1 ./bench 1 20000`

## 开发环境

本仓库提供 `.devcontainer/devcontainer.json`，在 GitHub Codespaces 中打开即可自动安装依赖。
