#include "ChatServer.hpp"
#include "ChatService.hpp"
#include <iostream>
#include <signal.h>

void resetHandler(int)
{
    chat::service::ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, resetHandler);

    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 2396);
    chat::ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}