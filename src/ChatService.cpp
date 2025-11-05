// clang-format off
#include "ChatService.hpp"

#include "UserModel.hxx"
#include "OfflineMsgModel.hxx"
#include "FriendModel.hxx"
#include "GroupModel.hxx"

#include "muduo/base/Logging.h"
#include "Common.h"

#include "base.pb.h"
#include "user.pb.h"
#include "friend.pb.h"
#include "group.pb.h"
#include "message.pb.h"
#include "notify.pb.h"  // no use

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
    msgHandlerMap_[EnMsgType::OFFLINE_REQ]          = std::bind(&ChatService::offline,      this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::LOGIN_REQ]            = std::bind(&ChatService::login,        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::REGIST_REQ]           = std::bind(&ChatService::regist,       this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::SINGLE_CHAT_MSG]      = std::bind(&ChatService::singleChat,   this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::ADD_FRIEND_REQ]       = std::bind(&ChatService::addFriend,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::CRET_GROUP_REQ]       = std::bind(&ChatService::createGroup,  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::JOIN_GROUP_REQ]       = std::bind(&ChatService::joinGroup,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::GET_USER_INFO_REQ]    = std::bind(&ChatService::getUserInfo,  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::GET_FRIEND_LIST_REQ]  = std::bind(&ChatService::getFriendList,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    msgHandlerMap_[EnMsgType::GET_OFFLINE_MSG_REQ]  = std::bind(&ChatService::getOfflineMsg,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // TODO: 或者将 Model_ 设计为 工具类？
    userModel_          = std::make_unique<chat::model::UserModel>();
    offlineMsgModel_    = std::make_unique<chat::model::OfflineMsgModel>();
    friendModel_        = std::make_unique<chat::model::FriendModel>();
    groupModel_         = std::make_unique<chat::model::GroupModel>();

    if(redis_mq_.connect())
        // 设置上报消息的回调
        redis_mq_.init_notify_handler(
            std::bind(&ChatService::handleMQSubscribeMessage,this, std::placeholders::_1, std::placeholders::_2));
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
    stark_im::UserLoginReq req;
    req.ParseFromString( js["body"].dump() );
    int userid = std::stoi(req.userid());
    std::string password = req.password();

    // 2.业务处理 - 并构造响应
    std::unique_ptr<chat::User> user = userModel_->query(userid);

    stark_im::UserLoginRsp rsp;
    if(user != nullptr && user->password() == password)
    {
        if(user->state() == "online")
        {
            // 用户已经登录，不允许重复登录
            rsp.set_success(false);
            rsp.set_errcode(ERR_REPEAT);
            rsp.set_errmsg("user state is online, can't to login again");
        }
        else
        {
            // 登录成功 - 维护这个用户的连接
            {
                std::lock_guard<std::mutex> lock(userConnMapMutex_);
                userConnMap_[user->userid()] = conn;
            }

            redis_mq_.subscribe(userid); // 订阅发给自己的消息

            // 登录成功 - 更新用户状态信息
            user->setState("online");
            userModel_->update(*user);

            // 登录成功 - 返回响应
            rsp.set_success(true);
        }
    }
    else
    {
        // 登录失败 - 用户名或密码错误
        rsp.set_success(false);
        rsp.set_errcode(ERR_NOT_MATCH);
        rsp.set_errmsg("username or password no match");
    }
    nlohmann::json response;   
    response["type"] = LOGIN_RES;
    response["body"] = rsp.SerializeAsString();

    // 3.发送响应
    conn->send(response.dump());
}

// 注册时，接收name、password
void chat::service::ChatService::regist(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1. 解析请求   
    stark_im::UserRegisterReq req;
    req.ParseFromString( js["body"].dump() );
    std::string username = req.username();
    std::string password = req.password();

    // 2. 业务处理
    User user;
    user.setUsername(username);
    user.setPassword(password);
    bool isexisted = (userModel_->query(username) != nullptr);
    bool isok = isexisted? false : userModel_->insert(user);

    // 3. 构造响应
    stark_im::UserRegisterRsp rsp;
    if(!isexisted)
    {
        if(isok)
        {
            // 注册成功
            rsp.set_success(true);          
        }
        else 
        {
            // 注册失败
            rsp.set_success(false);
            rsp.set_errcode(ERR_SERVICE);
            rsp.set_errmsg("sql insert error");
        }
    }
    else
    {
        // 注册失败
        rsp.set_success(false);
        rsp.set_errcode(ERR_REPEAT);
        rsp.set_errmsg("user has existed");
    }
    nlohmann::json response;    
    response["type"] = REGIST_RES;
    response["body"] = rsp.SerializeAsString();

    // 4. 发送响应
    conn->send(response.dump());
}

// 为了兼顾异常退出、正常下线，这里不使用js、只是用conn来找到对应的userid
void chat::service::ChatService::offline(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    std::optional<int> userid;
    stark_im::UserLogoutReq req;
    if(js.contains("body"))
    {
        req.ParseFromString(js["body"].dump());
        userid = std::stoi(req.userid());
    }
    else 
    {
        std::lock_guard<std::mutex> lock(userConnMapMutex_);
        for(const auto&[uid, c] : userConnMap_)
        {
            if(c == conn){
                userid = uid;
                break;
            }
        }
    }   

    if(userid.has_value())
    {
        // 取消订阅关于自己的消息
        redis_mq_.unsubscribe(userid.value());

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
    stark_im::FriendAddReq req;
    req.ParseFromString(js["body"].dump());
    int userid      = std::stoi(req.userid());
    int friendid    = std::stoi(req.friendid());

    // 2. 业务处理
    bool isfindok = userModel_->query(friendid) != nullptr;
    bool isaddok = isfindok? friendModel_->insert(userid, friendid) : isfindok;

    // 3. 构造响应
    stark_im::FriendAddRsp rsp;
    if(isfindok)
    {
        if(isaddok)
        {
            rsp.set_success(true);
        }
        else 
        {
            rsp.set_success(false);
            rsp.set_errcode(ERR_SERVICE);
            rsp.set_errmsg("sql insert error!");
        }
    }
    else
    {
        rsp.set_success(false);
        rsp.set_errcode(ERR_NOT_EXIST);
        rsp.set_errmsg("user not exists!");
    }
    nlohmann::json response;
    response["type"] = ADD_FRIEND_RES;
    response["data"] = rsp.SerializeAsString();

    // 4. 发送响应
    conn->send(response.dump());
}

// 创建群组业务
void chat::service::ChatService::createGroup(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1.解析请求
    stark_im::CreatGroupReq req;
    req.ParseFromString(js["body"].dump());
    int userid = std::stoi(req.userid());
    std::string groupname = req.groupname();
    std::string description = req.descrition();

    // 2.业务处理
    Group group;
    group.setGroupname(groupname);
    group.setDescription(description);
    bool iscreated = groupModel_->create(group);
    bool isjoined = iscreated ? groupModel_->join(userid, group.groupid(), "creator") : iscreated;

    // 3.构造响应
    stark_im::CreatGroupRsp rsp;
    if(iscreated)
    {
        if(isjoined){
            // 创建并加入成功
            rsp.set_success(true);
            rsp.set_groupid(std::to_string(group.groupid()));
        }
        else
        {
            rsp.set_success(false);
            rsp.set_errcode(ERR_SERVICE);
            rsp.set_errmsg("join error!");
        }
    }
    else
    {
        rsp.set_success(false);
        rsp.set_errcode(ERR_SERVICE);
        rsp.set_errmsg("create error!");
    }
    nlohmann::json response;
    response["type"] = CRET_GROUP_RES;
    response["body"] = rsp.SerializeAsString();
    
    // 4.发送响应
    conn->send(response.dump());
}

// 加入群组业务
void chat::service::ChatService::joinGroup(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1. 解析请求
    stark_im::JoinGroupReq req;
    req.ParseFromString(js["body"].dump());
    int userid = std::stoi(req.userid());
    int groupid = std::stoi(req.groupid());

    // 2. 业务处理
    bool isjoined = groupModel_->join(userid, groupid, "normal");

    // 3. 构造响应
    stark_im::JoinGroupRsp rsp;
    if(isjoined)
    {
        rsp.set_success(true);
    }
    else 
    {
        rsp.set_success(false);
        rsp.set_errcode(ERR_SERVICE);
        rsp.set_errmsg("join group error(maybe group not existed)");
    }
    nlohmann::json response;
    response["type"] = JOIN_GROUP_RES;
    response["body"] = rsp.SerializeAsString();

    // 4. 发送响应
    conn->send(response.dump());
}


void chat::service::ChatService::singleChat(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    stark_im::MessageSendReq req;
    req.ParseFromString(js["body"]);
    
    stark_im::MessageInfo msgInfo;
    int toid = std::stoi(msgInfo.toid());
    // if(userModel_->query(toid) == nullptr)
    // {
    //     // 用户不存在
    //     return;
    // }
    nlohmann::json msgcontent;
    msgcontent["from"] = msgInfo.fromid();
    msgcontent["to"] = msgInfo.toid();
    msgcontent["content"] = msgInfo.content();

    std::unordered_map<int, muduo::net::TcpConnectionPtr>::iterator iter;
    {
        std::lock_guard<std::mutex> lock(userConnMapMutex_);
        iter = userConnMap_.find(toid);
        if (iter != userConnMap_.end())
        {
            // toid 在线、直接转发消息
            iter->second->send(msgcontent.dump()); // 是那个连接向对应的客户端发的，不是向那个连接发的
            return;
        }
    }

    // toid 在线，但不在同一台主机
    auto user = userModel_->query(toid);
    if(user->state() == "online")
    {
        redis_mq_.publish(toid, msgcontent.dump());
        return;
    }

    // toid 不在线、存储离线消息
    offlineMsgModel_->insert(toid, msgcontent.dump());
}

void chat::service::ChatService::groupChat(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    stark_im::MessageSendReq req;
    req.ParseFromString(js["body"]);

    stark_im::MessageInfo msgInfo;
    int userid = std::stoi(msgInfo.fromid());
    int groupid = std::stoi(msgInfo.toid());

    nlohmann::json msgcontent;
    msgcontent["from"] = msgInfo.fromid();
    msgcontent["content"] = msgInfo.content();

    std::vector<int> membersIdVec = groupModel_->query(userid, groupid);
    for(int id : membersIdVec)
    {
        msgcontent["to"] = id;
        {
            std::lock_guard<std::mutex> lock(userConnMapMutex_);
            auto it = userConnMap_.find(id); // 找id对应的连接
            if(it != userConnMap_.end())
            {
                it->second->send(msgcontent.dump());
                continue;
            }
        }
        // 其它操作可以不被锁锁着、与连接映射表无关了
        auto user = userModel_->query(id);
        if(user->state() == "online")
        {
            redis_mq_.publish(id, msgcontent.dump());
        }
        else 
        {
            offlineMsgModel_->insert(id, msgcontent.dump());
        }
    }
}

void chat::service::ChatService::getUserInfo(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{ 
    // 1.解析请求
    stark_im::GetUserInfoReq req;
    req.ParseFromString(js["body"].dump());
    int userid = std::stoi(req.userid());
       
    // 2.业务处理 - 并构造响应
    std::unique_ptr<chat::User> user = userModel_->query(userid);

    // 3.构造响应
    stark_im::GetUserInfoRsp rsp;
    if(user != nullptr)
    {
        rsp.set_success(true);
        rsp.set_userid(std::to_string(user->userid()));
        rsp.set_username(user->username());
        rsp.set_state(user->state());
    }
    else 
    {
        rsp.set_success(false);
        rsp.set_errcode(-1);
        rsp.set_errmsg("find failed, not existed");
    }
    nlohmann::json response;
    response["type"] = GET_USER_INFO_RES;
    response["body"] = rsp.SerializeAsString();

    // 4.发送响应
    conn->send(response.dump());
}

void chat::service::ChatService::getFriendList(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    // 1.解析请求
    stark_im::GetFriendListReq req;
    req.ParseFromString(js["body"]);
    int userid = std::stoi(req.userid());

    // 2.获取好友列表
    std::vector<chat::User> friends = friendModel_->query(userid);

    stark_im::GetFriendListRsp rsp;
    rsp.set_success(true);
    if(!friends.empty()){
        for(const auto& f: friends)
        {
            stark_im::UserInfo u;
            u.set_userid(std::to_string(f.userid()));
            u.set_username(f.username());
            u.set_state(f.state());
            rsp.mutable_friendlist()->Add(std::move(u));
        }        
    }

    // 3. 构造响应
    nlohmann::json response;
    response["type"] = GET_FRIEND_LIST_RES;
    response["body"] = rsp.SerializeAsString();

    conn->send(response.dump());
}

void chat::service::ChatService::getOfflineMsg(const muduo::net::TcpConnectionPtr & conn, const nlohmann::json & js, const muduo::Timestamp & time)
{
    stark_im::GetOfflineMsgReq req;
    req.ParseFromString(js["body"].dump());
    int userid = std::stoi(req.userid());

    // 处理离线消息
    std::vector<std::string> messages = offlineMsgModel_->query(userid);
    stark_im::GetOfflineMsgRsp rsp;
    rsp.set_success(true);
    if(!messages.empty()){
        for(auto& msg : messages)
        {
            nlohmann::json msgjson = nlohmann::json::parse(msg);

            stark_im::MessageInfo msginfo;
            msginfo.set_fromid(msgjson["from"]);
            msginfo.set_toid(msgjson["to"]);
            msginfo.set_content(msgjson["content"]);

            rsp.mutable_msglist()->Add(std::move(msginfo));
        }
        offlineMsgModel_->remove(userid);
    }

    nlohmann::json response;
    response["type"] = GET_OFFLINE_MSG_RES;
    response["body"] = rsp.SerializeAsString();
    
    conn->send(response.dump());
}

// 从redis消息队列中获取订阅的消息
void chat::service::ChatService::handleMQSubscribeMessage(int userid, std::string message)
{
    std::lock_guard<std::mutex> lock(userConnMapMutex_);
    auto it = userConnMap_.find(userid);
    if(it != userConnMap_.end())
    {
        // message: json格式
        // ["from"]、["to"]、[content]
        it->second->send(message);
        return;
    }

    // 存储该用户的离线消息 - 在获取消息的这一个瞬间，用户下线了，就得存起来
    offlineMsgModel_->insert(userid, message);
}
