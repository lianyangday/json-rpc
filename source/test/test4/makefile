CFLAG= -std=c++11 -I ../../../build/release-install-cpp11/include/
LFLAG= -L../../../build/release-install-cpp11/lib  -ljsoncpp -lmuduo_net -lmuduo_base -pthread
all: publish_client subscribe_client server
publish_client: publish_client.cpp
	g++  -g $(CFLAG) $^ -o  $@ $(LFLAG)
subscribe_client: subscribe.cpp
	g++  -g $(CFLAG) $^ -o  $@ $(LFLAG)
server: server.cpp
	g++  -g $(CFLAG) $^ -o  $@ $(LFLAG)