// clang-format off
#ifndef CHATSERVER_H
#define CHATSERVER_H

#include "muduo/base/Logging.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include <string>

namespace chat
{

class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(muduo::net::EventLoop* loop,
               const muduo::net::InetAddress& listenAddr,
               const std::string& nameArg);
    
    // 启动服务
    void start();

private:
    // 上报连接相关信息的回调函数
    void onConnection(const muduo::net::TcpConnectionPtr&);
    // 上报读写事件相关信息的回调函数
    void onMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer *, muduo::Timestamp);

    muduo::net::EventLoop* loop_;   // 指向事件循环对象的指针
    muduo::net::TcpServer* server_; // 组合的muduo库，实现服务器功能的类的对象
};

} // namesapce chat

#endif // CHATSERVER_H