// clang-format off
#include "ChatServer.hpp"
#include "ChatService.hpp"
#include "json.hpp"

#include <string>
#include <functional>

chat::ChatServer::ChatServer(muduo::net::EventLoop* loop,
               const muduo::net::InetAddress& listenAddr,
               const std::string& nameArg)
    : loop_(loop)
    , server_(new muduo::net::TcpServer(loop, listenAddr, nameArg)) 
{
    // 注册连接回调
    server_->setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    // 注册信息回调
    server_->setMessageCallback(
        std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // 设置线程数
    server_->setThreadNum(4);
}

void chat::ChatServer::start()
{
    server_->start();
}

void chat::ChatServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    // 连接处理

    // 客户端断开连接
    if(!conn->connected())
    {
        nlohmann::json _; // 占位的json
        chat::service::ChatService::instance()->offline(conn, _, muduo::Timestamp::now());
        conn->shutdown(); // muduo日志库会打印的
    }
}

void chat::ChatServer::onMessage(const muduo::net::TcpConnectionPtr &conn, 
                                 muduo::net::Buffer *buffer, 
                                 muduo::Timestamp time)
{
    // 业务处理
    
    // 数据读取、并反序列化
    std::string buf = buffer->retrieveAllAsString();
    
    try{
        nlohmann::json js = nlohmann::json::parse(buf); 
        // 通过 js["msg_type"] 调用对应的服务 -> handler(conn, js, time)
        // 完全解耦网络模块的代码和业务模块的代码
        int msgType = js["msgType"].get<int>();
        auto&& msgHandler = chat::service::ChatService::instance()->getHandler(msgType);
        msgHandler(conn, js, time);
    } catch(std::exception& e){
        // 直接捕获解决
        LOG_ERROR << e.what();
    }
}