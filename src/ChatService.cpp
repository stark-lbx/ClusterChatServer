// clang-format off
#include "ChatService.hpp"
#include "muduo/base/Logging.h"
#include "Common.h"
#include "UserModel.hxx"
#include "OfflineMsgModel.hxx"
#include "FriendModel.hxx"

#include <optional>
#include <map>

chat::service::ChatService* chat::service::ChatService::instance()
{
    static ChatService service;
    return &service;
}

chat::service::ChatService::ChatService()
{
    // 注册对应的消息类型的回调函数
    msgHandlerMap_[EnMsgType::OFFLINE_REQ]      = std::bind(&ChatService::offline,      this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::LOGIN_REQ]        = std::bind(&ChatService::login,        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::REGIST_REQ]       = std::bind(&ChatService::regist,       this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::SINGLE_CHAT_MSG]  = std::bind(&ChatService::singleChat,   this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::ADD_FRIEND_REQ]   = std::bind(&ChatService::addFriend,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);


    // TODO: 或者将 Model_ 设计为 工具类？
    userModel_          = std::make_unique<chat::model::UserModel>();
    offlineMsgModel_    = std::make_unique<chat::model::OfflineMsgModel>();
    friendModel_        = std::make_unique<chat::model::FriendModel>();
}

chat::service::MsgHandler chat::service::ChatService::getHandler(int msgType)
{
    // 记录错误日志, msgid没有对应的事件处理回调
    std::unordered_map<int, MsgHandler>::iterator handlerIter = msgHandlerMap_.find(msgType);
    if(handlerIter == msgHandlerMap_.end())
    {
        // LOG_ERROR << "MsgType(" << msgType << ") is invalid! chat_service can't support!";
        // return MsgHandler();
        return [msgType](const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time){
            LOG_ERROR << "MsgType(" << msgType << ") is invalid! chat_service can't support!";
        };
    }
    return handlerIter->second;
}

void chat::service::ChatService::reset()
{
    // 把所有online的用户设置为offline
    userModel_->offlineAll();
}

void chat::service::ChatService::login(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 登录时，用户输入账户密码

    int userid = js["userid"].get<int>();
    std::string password = js["password"];

    std::unique_ptr<chat::User> user = userModel_->query(userid);

    nlohmann::json response;   
    response["msgType"] = LOGIN_RES;
    if(user != nullptr && user->password() == password)
    {
        if(user->state() == "online")
        {
            // 用户已经登录，不允许重复登录
            response["errno"] = 1;
            response["errmsg"] = "user state is online, can't to login again";
        }
        else
        {
            // 登录成功 - 维护这个用户的连接
            {
                std::lock_guard<std::mutex> lock(userConnMapMutex_);
                userConnMap_[user->userid()] = conn;
            }

            // 登录成功 - 更新用户状态信息
            user->setState("online");
            userModel_->update(*user);

            // 登录成功 - 返回响应
            response["errno"] = 0;
            response["errmsg"] = "";
            response["userid"] = user->userid();
            response["username"] = user->username();
            // response["loginSessionId"] = createUuid();

            // 登录成功 - 处理离线消息
            std::vector<std::string> messages = offlineMsgModel_->query(userid);
            if(!messages.empty()){
                response["offlinemsglist"] = messages;
                offlineMsgModel_->remove(userid);
            }

            // 登录成功 - 获取好友列表
            std::vector<chat::User> friends = friendModel_->query(userid);
            if(!friends.empty()){
                std::vector<std::string> friends_js;

                for(const auto& f: friends)
                {
                    nlohmann::json js;
                    js["userid"] = f.userid();
                    js["username"] = f.username();
                    js["state"] = f.state();
                    friends_js.emplace_back(js.dump());
                }

                response["friendlist"] = friends_js;
            }
        }
    }
    else
    {
        // 登录失败 - 用户名或密码错误
        response["errno"] = 2;
        response["errmsg"] = "username or password no match";
    }
    conn->send(response.dump());
}

void chat::service::ChatService::regist(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 注册只需要填name、password
    std::string username = js["username"];
    std::string password = js["password"];

    User user;
    user.setUsername(username);
    user.setPassword(password);
    bool isok = userModel_->insert(user);

    nlohmann::json response;    
    response["msgType"] = REGIST_RES;
    if(isok)
    {
        // 注册成功
        response["errno"] = 0;
        response["errmsg"] = "";
        response["userid"] = user.userid();
    }
    else
    {
        // 注册失败
        response["errno"] = 1;
        response["errmsg"] = "sql insert error";
    }
    conn->send(response.dump());
}

void chat::service::ChatService::offline(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 为了兼顾异常退出、正常下线，这里不使用js、只是用conn来找到对应的userid
    std::optional<int> userid;
    if(js.contains("userid"))
    {
        userid = js["userid"].get<int>();
    }
    else 
    {
        std::lock_guard<std::mutex> lock(userConnMapMutex_);
        for(const auto&[u, c] : userConnMap_)
        {
            if(c == conn){
                userid = u;
                break;
            }
        }
    }   

    if(userid.has_value())
    {
        std::unique_ptr<chat::User> user = userModel_->query(userid.value());
        if(user != nullptr)
        {
            user->setState("offline");
            userModel_->update(*user);
        }
    }
}

void chat::service::ChatService::singleChat(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    int toid = js["to"].get<int>();

    std::unordered_map<int, muduo::net::TcpConnectionPtr>::iterator iter;
    {
        std::lock_guard<std::mutex> lock(userConnMapMutex_);
        iter = userConnMap_.find(toid);
        if (iter != userConnMap_.end())
        {
            // toid 在线、直接转发消息
            iter->second->send(js.dump()); // 是那个连接向对应的客户端发的，不是向那个连接发的
            return;
        }
    }
    // toid 不在线、存储离线消息
    offlineMsgModel_->insert(toid, js.dump());
}

void chat::service::ChatService::addFriend(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    int userid = js["userid"].get<int>();
    int friendid   = js["friendid"].get<int>();

    nlohmann::json response;
    response["msgType"] = ADD_FRIEND_RES;
    if(!userModel_->query(friendid))
    {
        response["errno"] = 1;
        response["errmsg"] = "user not exists!";
    }
    else
    {
        if(!friendModel_->insert(userid, friendid))
        {
            response["errno"] = 2;
            response["errmsg"] = "sql insert error!";
        }
        else 
        {
            response["errno"] = 0;
            response["errmsg"] = "";
        }
    }
    conn->send(response.dump());
}
