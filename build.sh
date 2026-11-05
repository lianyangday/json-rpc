#!/usr/bin/env bash
# 一键构建：muduo -> 各测试用例 -> 压测客户端
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MUDUO_SRC="$ROOT/muduo-master"
JOBS="${JOBS:-$(nproc)}"

find_muduo_lib() {
    find "$ROOT" -name 'libmuduo_net.a' -print -quit 2>/dev/null | xargs -r dirname
}

echo "[1/3] 编译 muduo ..."
MUDUO_LIB="$(find_muduo_lib)"
if [ -z "$MUDUO_LIB" ]; then
    ( cd "$MUDUO_SRC" && ./build.sh -j"$JOBS" )
    MUDUO_LIB="$(find_muduo_lib)"
fi
if [ -z "$MUDUO_LIB" ]; then
    echo "错误：找不到 libmuduo_net.a，muduo 构建失败" >&2
    exit 1
fi
echo "      muduo 库目录: $MUDUO_LIB"

# 头文件直接用 muduo 源码树（<muduo/net/...> 可解析）
MUDUO_INC="$MUDUO_SRC"
# -I$ROOT/source/test 兜底：test1/test_server.cpp 里写的是 ../common（比其它用例少一级）
CXXFLAGS="-std=c++11 -O2 -g -I$MUDUO_INC -I$ROOT/source -I$ROOT/source/test"
LDFLAGS="-L$MUDUO_LIB -ljsoncpp -lmuduo_net -lmuduo_base -pthread"

echo "[2/3] 编译测试用例 ..."
build_exe() {   # $1=目录 $2=输出名 $3=源文件
    echo "      $1/$2"
    g++ $CXXFLAGS "$1/$3" -o "$1/$2" $LDFLAGS
}

build_exe source/test/test1 server         test_server.cpp
build_exe source/test/test1 client         test_client.cpp
build_exe source/test/test2 server         test_server.cpp
build_exe source/test/test2 client         test_client.cpp
build_exe source/test/test3 registry       registry_server.cpp
build_exe source/test/test3 server         test_server.cpp
build_exe source/test/test3 client         test_client.cpp
build_exe source/test/test4 server         server.cpp
build_exe source/test/test4 publish_client publish_client.cpp
build_exe source/test/test4 subscribe      subscribe.cpp

echo "[3/3] 编译压测客户端 ..."
g++ $CXXFLAGS "$ROOT/bench.cpp" -o "$ROOT/bench" $LDFLAGS

echo "完成。"
echo "  运行示例: ./source/test/test2/server  &&  ./source/test/test2/client"
echo "  压测示例: ./bench 1 20000 127.0.0.1 8080"
