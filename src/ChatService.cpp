// clang-format off
#include "ChatService.hpp"

#include "UserModel.hxx"
#include "OfflineMsgModel.hxx"
#include "FriendModel.hxx"
#include "GroupModel.hxx"

#include "muduo/base/Logging.h"
#include "Common.h"

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
    msgHandlerMap_[EnMsgType::CRET_GROUP_REQ]   = std::bind(&ChatService::createGroup,  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::JOIN_GROUP_REQ]   = std::bind(&ChatService::joinGroup,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // TODO: 或者将 Model_ 设计为 工具类？
    userModel_          = std::make_unique<chat::model::UserModel>();
    offlineMsgModel_    = std::make_unique<chat::model::OfflineMsgModel>();
    friendModel_        = std::make_unique<chat::model::FriendModel>();
    groupModel_         = std::make_unique<chat::model::GroupModel>();
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

// 登录时，用户输入账户密码
void chat::service::ChatService::login(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1.解析请求
    int userid = js["userid"].get<int>();
    std::string password = js["password"];

    // 2.业务处理 - 并构造响应
    std::unique_ptr<chat::User> user = userModel_->query(userid);

    nlohmann::json response;   
    response["msgType"] = LOGIN_RES;
    if(user != nullptr && user->password() == password)
    {
        if(user->state() == "online")
        {
            // 用户已经登录，不允许重复登录
            response["errno"] = ERR_REPEAT;
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
            response["errno"] = SUCCESS;
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
        response["errno"] = ERR_NOT_MATCH;
        response["errmsg"] = "username or password no match";
    }
    
    // 3.发送响应
    conn->send(response.dump());
}

// 注册时，接收name、password
void chat::service::ChatService::regist(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1. 解析请求   
    std::string username = js["username"];
    std::string password = js["password"];

    // 2. 业务处理
    User user;
    user.setUsername(username);
    user.setPassword(password);
    bool isexisted = (userModel_->query(username) != nullptr);
    bool isok = isexisted? false : userModel_->insert(user);

    // 3. 构造响应
    nlohmann::json response;    
    response["msgType"] = REGIST_RES;
    if(!isexisted)
    {
        if(isok)
        {
            // 注册成功
            response["errno"] = SUCCESS;
            response["errmsg"] = "";
            response["userid"] = user.userid();            
        }
        else 
        {
            // 注册失败
            response["errno"] = ERR_SERVICE;
            response["errmsg"] = "sql insert error";
        }
    }
    else
    {
        // 注册失败
        response["errno"] = ERR_REPEAT;
        response["errmsg"] = "user has existed";
    }

    // 4. 发送响应
    conn->send(response.dump());
}

// 为了兼顾异常退出、正常下线，这里不使用js、只是用conn来找到对应的userid
void chat::service::ChatService::offline(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
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

void chat::service::ChatService::addFriend(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1. 解析请求
    int userid = js["userid"].get<int>();
    int friendid   = js["friendid"].get<int>();

    // 2. 业务处理
    bool isfindok = userModel_->query(friendid) != nullptr;
    bool isaddok = isfindok? friendModel_->insert(userid, friendid) : isfindok;

    // 3. 构造响应
    nlohmann::json response;
    response["msgType"] = ADD_FRIEND_RES;
    if(isfindok)
    {
        if(isaddok)
        {
            response["errno"] = SUCCESS;
            response["errmsg"] = "";
        }
        else 
        {
            response["errno"] = ERR_SERVICE;
            response["errmsg"] = "sql insert error!";            
        }
    }
    else
    {
        response["errno"] = ERR_NOT_EXIST;
        response["errmsg"] = "user not exists!";
    }

    // 4. 发送响应
    conn->send(response.dump());
}

// 创建群组业务
void chat::service::ChatService::createGroup(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1.解析请求
    int userid = js["userid"].get<int>();
    std::string groupname = js["groupname"];
    std::string description = js["description"];

    // 2.业务处理
    Group group;
    group.setGroupname(groupname);
    group.setDescription(description);
    bool iscreated = groupModel_->create(group);
    bool isjoined = iscreated ? groupModel_->join(userid, group.groupid(), "creator") : iscreated;

    // 3.构造响应
    nlohmann::json response;
    response["msgType"] = CRET_GROUP_RES;
    if(iscreated)
    {
        if(isjoined){
            // 创建并加入成功
            response["errno"] = SUCCESS;
            response["errmsg"] = "";
            response["groupid"] = group.groupid();
        }
        else
        {
            response["errno"] = ERR_SERVICE;
            response["errmsg"] = "join error!";
        }
    }
    else
    {
        response["errno"] = ERR_SERVICE;
        response["errmsg"] = "create error!";
    }
    
    // 4.发送响应
    conn->send(response.dump());
}

// 加入群组业务
void chat::service::ChatService::joinGroup(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1. 解析请求
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();

    // 2. 业务处理
    bool isjoined = groupModel_->join(userid, groupid, "normal");

    // 3. 构造响应
    nlohmann::json response;
    response["msgType"] = JOIN_GROUP_RES;
    if(isjoined)
    {
        response["errno"] = SUCCESS;
        response["errmsg"] = "";
    }
    else 
    {
        response["errno"] = ERR_SERVICE;
        response["errmsg"] = "join group error(maybe group not existed)";
    }

    // 4. 发送响应
    conn->send(response.dump());
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

void chat::service::ChatService::groupChat(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();

    std::vector<int> membersIdVec = groupModel_->query(userid, groupid);
    for(int id : membersIdVec)
    {
        {
            std::lock_guard<std::mutex> lock(userConnMapMutex_);
            auto it = userConnMap_.find(id); // 找id对应的连接
            if(it != userConnMap_.end())
            {
                it->second->send(js.dump());
                continue;
            }
        }
        // 插入时可以不被锁锁着、与连接映射表无关了
        offlineMsgModel_->insert(id, js.dump());
    }
}
