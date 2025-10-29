// clang-format off
#include "ChatService.hpp"
#include "muduo/base/Logging.h"
#include "Common.hxx"
#include "UserModel.hxx"


chat::service::ChatService* chat::service::ChatService::instance()
{
    static ChatService service;
    return &service;
}

chat::service::ChatService::ChatService()
{
    // 注册对应的消息类型的回调函数
    msgHandlerMap_[EnMsgType::LOGIN_MSG_REQ]    = std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::REGIST_MSG_REQ]   = std::bind(&ChatService::regist, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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

void chat::service::ChatService::login(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 登录时，用户输入账户密码

    int userid = js["userid"].get<int>();
    std::string password = js["password"];

    std::unique_ptr<chat::User> user = userModel_->query(userid);

    nlohmann::json response;   
    response["msgType"] = LOGIN_MSG_RES;
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
            // 登录成功 - 更新用户状态信息
            user->setState("online");
            userModel_->update(*user);

            response["errno"] = 0;
            response["errmsg"] = "";
            response["userid"] = user->userid();
            response["username"] = user->username();
            // response["loginSessionId"] = createUuid();
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
    response["msgType"] = REGIST_MSG_RES;
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
