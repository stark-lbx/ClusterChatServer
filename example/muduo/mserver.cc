#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
// using namespace std;
// using namespace muduo;
// using namespace muduo::net;

// 基于muduo网络库开发服务器程序
// 1. 组合TcpServer对象
// 2. 创建EventLoop事件循环对象的指针
// 3. 明确TcpServer的构造参数
// 4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
// 5. 设置合适的服务端线程数，muduo库会自己分配IO线程和worker线程
class ChatServer
{
public:
    ChatServer(muduo::net::EventLoop *loop,               // 事件循环
               const muduo::net::InetAddress &listenAddr, // IP + port
               const std::string &nameArg)                // server_name
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开的回调
        _server.setConnectionCallback(
            std::bind(&ChatServer::onConnection, this, std::placeholders::_1));

        // 给服务器注册用户读写事件的回调
        _server.setMessageCallback(
            std::bind(&ChatServer::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));

        // 设置服务器端的线程数 1个IO线程、3个worker线程
        _server.setThreadNum(4);
    }

    // 开启服务器的事件循环
    void start() { _server.start(); }

private:
    // 处理用户的连接与断开：epoll、listenfd、accept
    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if (conn->connected())
            std::cout << conn->peerAddress().toIpPort() << "-> online\n";
        else
        {
            std::cout << conn->peerAddress().toIpPort() << "-> offline\n";
            conn->shutdown();
            // _loop->quit();
        }
    }

    // 处理用户读写事件（echo回显）
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buffer, muduo::Timestamp time)
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data: " << buf << " time:" << time.toString() << "\n";
        conn->send(buf);
    }

    muduo::net::TcpServer _server;
    muduo::net::EventLoop *_loop;
};

int main()
{
    muduo::net::EventLoop loop; // epoll_create
    muduo::net::InetAddress addr("127.0.0.1", 8888);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl->epoll
    loop.loop();    // epoll_wait以阻塞方式等待用户连接, 已连接的用户的读写事件等

    return 0;
}