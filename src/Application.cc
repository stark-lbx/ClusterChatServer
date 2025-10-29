#include "ChatServer.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 2396);
    chat::ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}