#ifndef COMMON_H
#define COMMON_H

// 前后端交互须知


// 消息类型
enum EnMsgType{
    OFFLINE_MSG = 0,    // 离线请求    
    
    LOGIN_MSG_REQ,      // 登录消息
    LOGIN_MSG_RES,      // 登录响应
    
    REGIST_MSG_REQ,     // 注册消息
    REGIST_MSG_RES,     // 注册响应

    SINGLE_CHAT_MSG,    // 单聊消息
};


/** 请求报文(json 格式)
 * "msgType" : EnMsgType->int,
 * 
 * if is login_req
 *      "userid" : id->int;
 *      "password" : pwd->string;
 *      "offlinemsglist" : ["",""]->vector<string>;
 * 
 * if is regsit_req
 *      "username" : name->string;
 *      "password" : pwd->string;
 * 
 * if is offline
 *      "userid" : id->int
 * 
 * if is single_chat
 *      "userid" : id->int
 *      "username" : name->string
 *      "to" : id->int 
 *      "msg" : msg->string
 */



/** 响应报文(json 格式)
 * "msgType" : EnMsgType->int,
 * "errno" : errcode->int,
 * "errmsg" : error description
 * 
 * if is login_res
 *      "userid" : id->int,
 *      "username" : name->string,
 * 
 * if is  regist_res
 *      "userid" : id->int
 */

#endif