#ifndef COMMON_H
#define COMMON_H

// 前后端交互须知

// 消息类型
enum EnMsgType{
    LOGIN_MSG_REQ = 1,  // 登录消息
    LOGIN_MSG_RES,  // 登录响应
    REGIST_MSG_REQ,      // 注册消息
    REGIST_MSG_RES, // 注册响应
};

#endif