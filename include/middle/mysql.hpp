// clang-format off
#ifndef MYSQL_COMPLECTION_H
#define MYSQL_COMPLECTION_H

#include <mysql/mysql.h>
#include <cstdio>
#include <ctime>

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>
#include <chrono>


// 对外隐藏
namespace
{
namespace conf
{
static unsigned int SERVER_PORT = 3306;
static std::string SERVER_IP = "127.0.0.1";
static std::string USER_NAME = "root";
static std::string PASSWORD = "123456";
static std::string DB_NAME = "chatserver";

static int INIT_SIZE = 10;
static int MAX_SIZE = 1024;
static int MAX_IDLE_TIME = 60*1000;       // ms
static int CONNECTION_TIMEOUT = 100;      // ms
}

class MySQL
{
public:
    MySQL(){ conn_ = ::mysql_init(nullptr); }
    
    ~MySQL(){ if(conn_) mysql_close(conn_); }

    bool connect(std::string serverip, unsigned int serverport, std::string username, std::string password, std::string dbname) {
        MYSQL *p = ::mysql_real_connect(conn_, 
                                        serverip.c_str(),
                                        username.c_str(),
                                        password.c_str(),
                                        dbname.c_str(),
                                        serverport, nullptr, 0);
        if(p != nullptr)
        {
            // 设置中文信息(C/C++代码默认的编码是ASCII码、如果不设置，从mysql拉下来的中文显示'?')
            ::puts("[db::mysql::connect] success!");
            ::mysql_query(conn_, "set names gbk");
        }
        else 
        {
            ::puts("[db::mysql::connect] failed!");
            ::printf("\t[%s:%d - %s:%s - %s] %p\n",serverip.c_str(), serverport, username.c_str(), password.c_str(), dbname.c_str(), conn_);
        }
        return p != nullptr;
    }

    bool update(std::string sql) {
        // mysql_query: 0 表示正常, 非 0 表示错误
        if(::mysql_query(conn_, sql.c_str()))
        {
            ::printf("[db::mysql::update] error!\n\t%s\n", sql.c_str());
            return false;
        }
        return true;
    }
    
    MYSQL_RES* query(std::string sql){
        if(::mysql_query(conn_, sql.c_str()))
        {
            ::printf("[db::mysql::query] error!\n\t%s\n", sql.c_str());
            return nullptr;
        }
        return ::mysql_use_result(conn_);
    }

    MYSQL* connection() const{ return conn_; }

    // 刷新一下连接的起始的空闲时间点
    void refreshTime(){startTime_ = ::clock();}
    // 获取当前连接的空闲时间点
    clock_t getAlivetime(){return clock()-startTime_;}
private:
    MYSQL* conn_;
    clock_t startTime_; //记录进入空闲状态后的起始时间
};
}

namespace chat
{
namespace db
{
/**
 * 为了提高MySQL数据库的访问瓶颈，除了在服务器端增加缓存服务器 缓存常用的数据之外，还可以增加连接池，来提高MySQL Server的访问效率；
 * 在高并发的情况下，大量的 TCP 三次握手、MySQL Server连接认证、MySQL Server关闭连接回收资源和 TCP 四次挥手所耗费的性能时间也是很明显的；
 * 增加连接池就是为了减少这一部分的性能损耗。
 * 
 * 在市面上比较流行的连接池包括阿里的druid，c3p0以及apache dbcp连接池，它们对于短时间内大量的数据库增删改查操作性能的提升是很明显的，但是它们的共同点都是，java实现。
 * 
 * 功能点：
 *  连接池一般包含了数据库连接所用的ip地址，port端口、用户名和密码以及其它的性能参数，例如：初始连接量、最大连接量、最大空闲使劲啊、连接超时时间等。
 *  初始连接量（initSize）：表示连接池预先会和MySQLServer创建initSize个数的connection连接，当应用发起MySQL访问时，不再创建和MySQL Server新的连接，而是直接从池中获取一个可用的连接即可，使用完成后，并不去释放connection，而是把当前connection再归还到连接池当中
 *  最大连接量（maxSize）：当并发访问 MySQL Server的请求增多时，初始连接量已经不够用了，此时会根据新的请求数量去创建更多的连接给应用去使用，但是新创建的连接数量上限是maxSize，不能无限制的创建连接，因为每一个连接都会占用socket资源，一般连接池和服务器程序是部署在一台主机上的，如果连接池占用过多的socket，那么服务器就接收不了过多的客户请求了
 *  最大空闲时间（maxIdleTime）：当访问MySQL的并发请求多了以后，连接池里面的连接数量会动态增加，上限是maxSize个，当这些连接用完再次归还到连接池当中，如果在指定的maxIdleTime内，这些新增的连接都没有再次被使用，就使新增的连接资源被回收
 *  连接超时时间（connTimeout）：当MySQL的并发请求量过大，连接池的连接数量已经到达maxSize了，而此时没有空闲的连接可以使用，那么此时应用从连接池获取连接无法成功，它通过阻塞的方式获取连接的时间如果超过connTimeout时间，那么就是获取连接失败，无法访问数据库
 * 技术点：
 *  单例模式、queue队列容器、C++11多线程编程、线程互斥、线程同步通信和unique_lock、基于CAS的原子整型、智能指针shared_ptr、lambda表达式、生产者-消费者模型。
 */

// 数据库连接池
class ConnectionPool
{
public:
    static ConnectionPool* instance()
    {
        static ConnectionPool cp;
        return &cp;
    }
    
    std::shared_ptr<MySQL> getConnection()
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        while(connectionQue_.empty())
        {
            if(std::cv_status::timeout == cvConsumer_.wait_for(lock, std::chrono::milliseconds(connTimeout_)))
            {
                ::printf("获取空闲连接超时... 获取连接失败\n");
                return nullptr;
            }
        }

        // shared_ptr智能指针析构时，会把connection资源delete掉，相当于调用connection的析构函数，close掉连接了
        // 这里需要自定义删除器，把connection直接归还到queue中
        std::shared_ptr<MySQL> ret(connectionQue_.front(), 
            [this](MySQL *pconn){
                using db::ConnectionPool;
                std::unique_lock<std::mutex> lock(this->queueMutex_);
                pconn->refreshTime();
                this->connectionQue_.push(pconn);
            }); // 删除的调用不在这里，需要加锁
        connectionQue_.pop();
        cvProducer_.notify_one(); // 通知生产者查看是否需要生产
        return ret;
    }

private:
    bool loadConfigure(){
        // 数据库配置信息
        ip_         = conf::SERVER_IP;
        port_       = conf::SERVER_PORT;
        username_   = conf::USER_NAME;
        password_   = conf::PASSWORD;
        dbname_     = conf::DB_NAME;
        
        // 连接池配置信息
        initSize_       = conf::INIT_SIZE;
        maxSize_        = conf::MAX_SIZE;
        maxIdleTime_    = conf::MAX_IDLE_TIME;
        connTimeout_    = conf::CONNECTION_TIMEOUT;

        return true;
    }
    
    // 运行在独立的线程中，专门负责生产新连接
    void produceConnectionTask()
    {
        for(;;){
            std::unique_lock<std::mutex> lock(queueMutex_);
            while(!connectionQue_.empty())
            {
                cvProducer_.wait(lock); //队列不空，此处生产线程进入等待状态
            }

            // 判断连接是否达到最大数
            if(connectionCounter_ < maxSize_)
            {
                auto* p = new MySQL(); 
                if(!p->connect(ip_, port_, username_, password_, dbname_)){
                    continue;
                }
                p->refreshTime();
                connectionQue_.push(std::move(p)); 
                connectionCounter_.fetch_add(1);
            }

            cvConsumer_.notify_all();   //通知消费者线程，可以消费连接了
        }
    }
    
    // 运行在独立的线程中，定时扫描空闲连接
    void scanConnectionTask()
    {
        for(;;)
        {
            // 先睡这么长时间，醒了看谁时间超了就移除
            std::this_thread::sleep_for(std::chrono::seconds(maxIdleTime_));
            
            while(connectionCounter_ > initSize_)
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                auto* p = connectionQue_.front();
                if(p->getAlivetime() >= maxIdleTime_)
                {
                    connectionQue_.pop();
                    delete p; // 释放连接
                    connectionCounter_.fetch_sub(1);
                }
                else 
                {
                    break; // 队列的空闲起始时间是根据先进先出的，所以队头是空闲最久的，如果队头都空闲太短，后面指定更短
                }
            }
        }
    }

    ConnectionPool()
        :queueMutex_(), connectionCounter_(0), cvConsumer_(), cvProducer_()
    {
        // 加载配置项
        if(!loadConfigure())
        {
            ::printf("[db::connection_poll::ctor] error!\n\t%s\n", "reason is loadConfigure");
        }

        for(int i=0; i<initSize_; ++i)
        {
            MySQL* p = new MySQL(); 
            if(!p->connect(ip_, port_, username_, password_, dbname_)){
                continue;
            } 
            p->refreshTime();

            connectionQue_.push(std::move(p)); // not must to lock in ctor
            connectionCounter_.fetch_add(1);
        }

        // 启动一个新线程，作为连接的生产者
        std::thread producer(std::bind(&ConnectionPool::produceConnectionTask, this));        
        producer.detach(); 

        // 启动一个新的定时线程，扫描 超过maxIdleTime 时间的空闲连接，进行连接回收
        std::thread scanner(std::bind(&ConnectionPool::scanConnectionTask, this));
        scanner.detach(); // 这里一定要加入后台
    }

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool operator=(const ConnectionPool&) = delete;

    std::string ip_;
    unsigned int port_;   // 3306
    std::string username_;
    std::string password_;
    std::string dbname_;
    
    int initSize_;      // 初始连接数
    int maxSize_;       // 最大连接数
    int maxIdleTime_;   // 最大空闲时间
    int connTimeout_;   // 连接超时时间

    std::queue<MySQL*> connectionQue_;  //存储MySQL连接的队列
    std::mutex queueMutex_; // 维护连接队列的线程安全互斥锁
    std::atomic_int connectionCounter_; // 记录连接所创建的MySQL连接的总数量
    std::condition_variable cvConsumer_; 
    std::condition_variable cvProducer_; // 设置条件变量，用于连接生产线程 和 连接消费线程 的通信
};

} // namespace db
} // namespace chat

#endif // MYSQL_COMPLECTION_H