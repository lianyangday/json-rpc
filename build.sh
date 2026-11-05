#!/usr/bin/env bash
# 一键构建：muduo -> 各测试用例 -> 压测客户端
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MUDUO_SRC="$ROOT/muduo-master"
INSTALL="$ROOT/build/release-install-cpp11"
JOBS="${JOBS:-$(nproc)}"

echo "[1/3] 编译 muduo ..."
if [ ! -f "$INSTALL/lib/libmuduo_net.a" ]; then
    ( cd "$MUDUO_SRC" && ./build.sh -j"$JOBS" )
    mkdir -p "$ROOT/build"
    cp -a "$MUDUO_SRC/build/release-install-cpp11" "$ROOT/build/" 2>/dev/null || true
else
    echo "      muduo 已构建，跳过"
fi

INC="$INSTALL/include"
LIB="$INSTALL/lib"
CXXFLAGS="-std=c++11 -O2 -g -I$INC -I$ROOT/source"
LDFLAGS="-L$LIB -ljsoncpp -lmuduo_net -lmuduo_base -pthread"

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
