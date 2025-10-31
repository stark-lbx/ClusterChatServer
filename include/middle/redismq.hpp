// clang-format off
#ifndef REDIS_COMPLECTION_H
#define REDIS_COMPLECTION_H

#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include "hiredis/hiredis.h"


namespace{
    std::string server = "127.0.0.1";
    int port = 6379;
}

namespace chat
{
namespace mq
{

// 基于redis的订阅发布实现的消息队列

class RedisMQ
{
public:
    using NotifyHandler = std::function<void(int, std::string)>;

    RedisMQ() : publish_context_(nullptr)
              , subscribe_context_(nullptr) {}
    ~RedisMQ()
    {
        if(publish_context_ != nullptr)
        {
            redisFree(publish_context_);
        }
        if(subscribe_context_ != nullptr)
        {
            redisFree(subscribe_context_);
        }
    }

    // 连接redis服务器
    bool connect()
    {
        publish_context_ = redisConnect(server.c_str(), port);
        if(publish_context_ == nullptr)
        {
            std::cerr << "mq->connect redis failed!" <<std::endl;
            return false;
        }

        subscribe_context_ = redisConnect(server.c_str(), port);
        if(subscribe_context_ == nullptr)
        {
            std::cerr << "mq->connect redis failed!" <<std::endl;
            return false;
        }

        // 监听通道上的事件，有消息给业务层上报
        std::thread([&]{
            observer_channel_message();
        }).detach();
        std::cout << "mq->connect redis-server success!" << std::endl;

        return true;
    }

    // 向redis指定的通道channel发布消息
    bool publish(int channel, const std::string& message)
    {
        redisReply* reply = (redisReply*)redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str());
        if(reply == nullptr)
        {
            std::cerr << "mq->publish redis-command failed!" <<std::endl;
            return false;
        }
        freeReplyObject(reply);
        return true;
    }
    
    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel)
    {
        /**
         * subscribe 命令本身会造成线程阻塞，等待通道里面发生消息，这里只做订阅通道，不接收通道消息
         * 通道消息的接收 专门在observer_channel_message 函数中的独立线程中进行。
         * 只负责发送命令、不阻塞接收redisserver响应消息，否则notifyMsg线程抢占响应资源。
         */
        if(REDIS_ERR == redisAppendCommand(subscribe_context_, "SUBSCRIBE %d", channel))
        {
            std::cerr << "mq->subscribe redis-command failed!" << std::endl;
            return false;
        }

        int done = 0;
        while (!done)
        {
            if(REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
            {
                std::cerr << "mq->subscribe redis-buffer-wirte failed!" << std::endl;
                return false;
            }

        }
        // redisGetReply

        return true;
    }
    
    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel)
    {        
        if(REDIS_ERR == redisAppendCommand(subscribe_context_, "UNSUBSCRIBE %d", channel))
        {
            std::cerr << "mq->unsubscribe redis-command failed!" << std::endl;
            return false;
        }
        
        int done = 0;
        while (!done)
        {
            if(REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
            {
                std::cerr << "mq->subscribe redis-buffer-wirte failed!" << std::endl;
                return false;
            }

        }
        return true;
    }

    // 在独立线程中接收订阅 通道中的消息
    void observer_channel_message()
    {
        redisReply* reply = nullptr;
        while(REDIS_OK == redisGetReply(subscribe_context_, (void**)&reply))
        {
            if(reply != nullptr 
            && reply->element[2] != nullptr
            && reply->element[2]->str != nullptr)
            {
                notify_message_handler_(::atoi(reply->element[1]->str), reply->element[2]->str);
            }
            freeReplyObject(reply);
            reply = nullptr;
        }
        std::cout << ">>>>>> observer_channel_message quit <<<<<<" << std::endl;
    }

    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(NotifyHandler fn)
    {
        notify_message_handler_ = std::move(fn);
    }

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