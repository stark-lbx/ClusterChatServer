#ifndef COMMON_H
#define COMMON_H

// 前后端交互须知


// 消息类型
enum EnMsgType{
    OFFLINE_REQ = 0,    // 离线请求    
    
    LOGIN_REQ,      // 登录消息
    LOGIN_RES,      // 登录响应
    
    REGIST_REQ,     // 注册消息
    REGIST_RES,     // 注册响应

    SINGLE_CHAT_MSG,    // 发送单聊消息

    ADD_FRIEND_REQ,     // 添加好友请求
    ADD_FRIEND_RES,     // 添加好友响应

    CRET_GROUP_REQ,     // 创建群组请求
    CRET_GROUP_RES,     // 创建群组响应

    JOIN_GROUP_REQ,     // 加入群组请求
    JOIN_GROUP_RES,     // 加入群组响应

    GET_USER_INFO_REQ,
    GET_USER_INFO_RES,

    GET_FRIEND_LIST_REQ,
    GET_FRIEND_LIST_RES,

    GET_OFFLINE_MSG_REQ,
    GET_OFFLINE_MSG_RES,
};

// 错误码枚举
enum ErrCode{
    ERR_SERVICE,        // sql语句错误或者其它错误（称为服务错误）

    ERR_REPEAT,         // 重复登录|重复注册
    ERR_NOT_EXIST,      // 用户不存在 | xx不存在
    ERR_NOT_MATCH,      // 用户名与密码不匹配（可能用户不存在, 但客户端不应该知道有没有这个用户，只应该知道账号或者密码输错了）
};

/** 双方消息格式
 *  {
 *      "type": xx,
 *      "body": "protobuf data"
 *  }
 * 
 *  离线消息存储格式
 *  {
 *      "from": fromid,
 *      "to": toid,
 *      "content": content-string
 *  }
 */
#endif