// clang-format off
#ifndef REDIS_COMPLECTION_H
#define REDIS_COMPLECTION_H

#include <string>
#include <cstdio>
#include <functional>
#include "hiredis.h"

namespace chat
{
namespace mq
{

// 基于redis的订阅发布实现的消息队列

class RedisMQ
{
public:
    using NotifyHandler = std::function<void(int, std::string)>;

    RedisMQ();
    ~RedisMQ();

    // 连接redis服务器
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, const std::string& message);
    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);
    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅 通道中的消息
    void observer_channel_message();

    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(NotifyHandler fn);

private:
    // hiredis 同步上下文对象，负责publish消息
    redisContext* publish_context_;
    // hiredis 同步上下文对象，负责subscribe消息
    redisContext* subscribe_context_;

    // 回调操作，收到订阅的消息，给service层上报
    NotifyHandler notify_message_handler_;
};

} // mq
} // chat

#endif