// clang-format off
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "muduo/net/TcpConnection.h"
#include "middle/json.hpp"
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

namespace chat
{
namespace model
{
    // 前置声明

    class UserModel;
    class OfflineMsgModel;
    class FriendModel;
    class GroupModel;
}

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
    MsgHandler getHandler(int msgType);
    void reset();

    void offline(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time);
    void login(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);
    void regist(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);

    void addFriend(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);

    void createGroup(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);

    void joinGroup(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);

    void singleChat(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);
    void groupChat(const muduo::net::TcpConnectionPtr& conn, const nlohmann::json& js, const muduo::Timestamp& time);


private:
    ChatService();
    
    // 单例模式
    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;

    // 消息类型 和 其对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> userConnMap_;
    std::mutex userConnMapMutex_;

    // 数据操作对象
    std::unique_ptr<model::UserModel> userModel_;
    std::unique_ptr<model::OfflineMsgModel> offlineMsgModel_;
    std::unique_ptr<model::FriendModel> friendModel_;
    std::unique_ptr<model::GroupModel> groupModel_;
};

} // namespace service
} // namespace chat

#endif // CHATSERVICE_H