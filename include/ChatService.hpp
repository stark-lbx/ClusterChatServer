// clang-format off
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "muduo/net/TcpConnection.h"
#include "json/json.hpp"
#include <unordered_map>
#include <functional>
#include <memory>

namespace chat
{
namespace model{class UserModel;}

namespace service
{

// 处理消息以及对应的 Handler 回调操作
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr&, 
                                      const nlohmann::json&, 
                                      const muduo::Timestamp&)>;

// 聊天服务器的 聊天服务 业务模块
class ChatService
{
public:
    static ChatService* instance();

    void login(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);
    void regist(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);

    MsgHandler getHandler(int msgType);
private:
    ChatService();
    
    // 单例模式
    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;

    // 消息类型 和 其对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 数据操作对象
    std::unique_ptr<chat::model::UserModel> userModel_;
};

} // namespace service
} // namespace chat

#endif // CHATSERVICE_H