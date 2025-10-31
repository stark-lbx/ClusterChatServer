# ClusterChatServer
集群聊天服务器
技术栈：Json、muduo、nginx、redis、mysql、cmake、github
更改技术栈：protobuf、muduo（httplib、brpc）、nginx、redis、mysql、cmake、github

项目需求：
    - 客户端新用户注册
    - 客户端用户登录
    - 添加好友、添加群组
    - 好友聊天
    - 群组聊天
    - nginx配置tcp负载均衡
    - 集群聊天系统支持客户端跨服务器通信

项目目标：
    - 掌握服务器的网络IO模块，业务模块、数据模块的分层设计思想
    - 掌握C++ muduo库网络库的编程和实现原理
    - 掌握Json的编程应用
    - 掌握nginx配置部署tcp负载均衡器的应用以及原理
    - 掌握服务器中间件的应用场景和基于发布-订阅的redis编程实践以及应用原理
    - 掌握CMake构建自动化编译环境
    - 掌握Github管理项目

开发环境：
    - (*) ubuntu-linux 环境
    - (*) cmake 环境
    - (*) json 开发库
    - (*) boost + muduo 网络库开发环境
    - (*) mysql 开发库
    - () redis 开发库

# Json
Json是一种轻量级的数据交换格式（序列化方式）。
Json采用完全独立于编程语言的文本格式来存储和表示数据。
简介和清晰的层次结构使得Json称为理想的数据交换语言。易于人阅读和编写，同时也易于机器解析和生成。
可代替的序列化方式：xml、protobuf
nlohmann/json.hpp (只有一个头文件)


# muduo
需要链接：
-L/usr/local/muduo/lib
libmuduo_net.so libmuduo_base.so libpthread.so

# cmake:
...
标准开源代码的工程目录结构：
project/
  |--- autobuild.sh         <!-- 自动构建脚本>
  |--- CMakeLists.txt       <!-- cmake管理文件>
  |--- bin/                 <!-- 输出可执行文件>
  |--- build/               <!-- 构建目录>
  |--- example/             <!-- 示例代码>
  |--- include/             <!-- 头文件>
  |--- lib/                 <!-- 库文件>
        |--- thrid/         <!-- 第三方依赖>
  |--- src/                 <!-- 源文件>

# mysql:
数据库环境搭建;

数据表设计

这个项目所涵盖的几个对象实体包括：
    用户 - 即服务面向的对象
    会话 - 即群组（两个人也是一个群组-单聊、 多个人也是群组-群聊）
    消息 - 在线消息可以由服务器直接转发给对方、离线消息存储在数据库等待对方上线后获取
各个实体之间的对应关系：
    用户与用户：好友关系；（friend表）
    用户与会话：从属关系；（groupuser表）
    会话与消息：包含关系；（不涉及历史消息存储服务）

- User表，包括userid、name、password、state（是否在线）
- Friend表，包括userid、friendid。（两个都是联合主键）
    【与User表有关，这么单独拿出来一张表设计 friend关系 可以进行User表的多个用户之间多对多的关系， 还可以灵活拓展单向好友和双向的区别】
- AllGroup表，包括groupid、groupname、groupdesc
    涉及到的所有的群组都维护在这个表内
- GroupUser表，包括groupid、userid、grouprole
    群组id、组员id、组内的角色（creator？normal？）
- OfflineMessage表，msgid、message（Json格式）
    离线消息存储

User表
    字段名      类型             描述            约束
    userid      int             用户id          自增主键（考虑 uuid。。。）
    username  varchar           用户名          非空+唯一
    password  varchar           密码            非空
    state     enum("on","off")  登录状态        默认="offline"

Friend表（又称为FriendShip表）：描述的是好友关系的一组
    字段名      类型         描述                约束
    userid      int         用户id      非空、联合主键（外键）
    friendid    int         好友id      非空、联合主键（外键）

AllGroup表：描述这个工程中所有的群组
    groupid     int         群组id              自增主键
    groupname   varchar     群组名              非空
    groupdesc   varchar     群组描述             默认 = ' '

GroupUser表：描述一个用户在哪个群组内，扮演着怎样的角色
    groupid     int             群组id              联合主键（关联AllGroup表）
    userid      int             用户id              联合主键（关联User表）
    grouprole   enum("c","n")   群内角色            默认 = "normal"         

OfflineMessage表：存储离线消息
    <!-- msgid       int         消息id              自增主键 -->
    userid      int         接收用户的id        联合主键  <!-- 在json格式中（即用户协议）就涵盖了消息的发送方、接收方 | 不行、不方便查询-->
    message     varchar()   离线消息             非空



```sql
create table User(
    userid int auto_increment primary key,
    username varchar(255) not null unique,
    password varchar(255) not null,
    state enum('offline', 'online') default 'offline'
);

create table Friend(
    userid int not null,
    friendid int not null,
    primary key (userid, friendid),
    foreign key (userid) references User(userid),
    foreign key (friendid) references User(userid)
);

create table AllGroup(
    groupid int auto_increment primary key,
    groupname varchar(255) not null,
    groupdesc varchar(512) default ''
);

create table GroupUser(
    groupid int not null,
    userid int not null,
    grouprole enum('creator','normal') default 'normal',
    primary key (groupid,userid),
    foreign key (groupid) references AllGroup(groupid),
    foreign key (userid) references User(userid)
);

create table OfflineMessage(
    userid int,
    message varchar(1024) not null,

    foreign key (userid) references User(userid)
);

```



# 项目模块定义
网络模块: muduo库开发ChatServer

业务模块: 业务层封装服务模块：ChatService

数据模块-mysql数据库：
    ORM框架：Object Relation Map-对象关系映射

<!-- 客户端：MVC: Model - View - Control -->.


# 项目结构（单机版本）
├── CMakeLists.txt
├── README.md
├── bin
│   ├── ChatServer
│   └── ClientRequest.json
├── build
│   ├── CMakeCache.txt
│   ├── CMakeFiles
│   │   ├── 3.22.1
│   │   │   ├── CMakeCCompiler.cmake
│   │   │   ├── CMakeCXXCompiler.cmake
│   │   │   ├── CMakeDetermineCompilerABI_C.bin
│   │   │   ├── CMakeDetermineCompilerABI_CXX.bin
│   │   │   ├── CMakeSystem.cmake
│   │   │   ├── CompilerIdC
│   │   │   │   ├── CMakeCCompilerId.c
│   │   │   │   ├── a.out
│   │   │   │   └── tmp
│   │   │   └── CompilerIdCXX
│   │   │       ├── CMakeCXXCompilerId.cpp
│   │   │       ├── a.out
│   │   │       └── tmp
│   │   ├── CMakeDirectoryInformation.cmake
│   │   ├── CMakeOutput.log
│   │   ├── CMakeTmp
│   │   ├── Makefile.cmake
│   │   ├── Makefile2
│   │   ├── TargetDirectories.txt
│   │   ├── cmake.check_cache
│   │   └── progress.marks
│   ├── Makefile
│   ├── cmake_install.cmake
│   └── src
│       ├── CMakeFiles
│       │   ├── CMakeDirectoryInformation.cmake
│       │   ├── ChatServer.dir
│       │   │   ├── Application.cc.o
│       │   │   ├── Application.cc.o.d
│       │   │   ├── ChatServer.cpp.o
│       │   │   ├── ChatServer.cpp.o.d
│       │   │   ├── ChatService.cpp.o
│       │   │   ├── ChatService.cpp.o.d
│       │   │   ├── DependInfo.cmake
│       │   │   ├── FriendModel.cxx.o
│       │   │   ├── FriendModel.cxx.o.d
│       │   │   ├── OfflineMsgModel.cc.o
│       │   │   ├── OfflineMsgModel.cc.o.d
│       │   │   ├── OfflineMsgModel.cxx.o
│       │   │   ├── OfflineMsgModel.cxx.o.d
│       │   │   ├── UserModel.cc.o
│       │   │   ├── UserModel.cc.o.d
│       │   │   ├── UserModel.cxx.o
│       │   │   ├── UserModel.cxx.o.d
│       │   │   ├── build.make
│       │   │   ├── cmake_clean.cmake
│       │   │   ├── compiler_depend.internal
│       │   │   ├── compiler_depend.make
│       │   │   ├── compiler_depend.ts
│       │   │   ├── depend.make
│       │   │   ├── flags.make
│       │   │   ├── link.txt
│       │   │   ├── model
│       │   │   │   ├── FriendModel.cxx.o
│       │   │   │   ├── FriendModel.cxx.o.d
│       │   │   │   ├── GroupModel.cxx.o
│       │   │   │   ├── GroupModel.cxx.o.d
│       │   │   │   ├── OfflineMsgModel.cxx.o
│       │   │   │   ├── OfflineMsgModel.cxx.o.d
│       │   │   │   ├── UserModel.cxx.o
│       │   │   │   └── UserModel.cxx.o.d
│       │   │   └── progress.make
│       │   └── progress.marks
│       ├── Makefile
│       └── cmake_install.cmake
├── example
│   ├── cmake
│   │   └── CMakeLists.txt
│   ├── json
│   │   ├── example
│   │   │   ├── TestJson.cpp
│   │   │   └── test
│   │   └── json.hpp
│   └── muduo
│       ├── CMakeLists.txt
│       ├── build
│       │   ├── CMakeCache.txt
│       │   ├── CMakeFiles
│       │   │   ├── 3.22.1
│       │   │   │   ├── CMakeCCompiler.cmake
│       │   │   │   ├── CMakeCXXCompiler.cmake
│       │   │   │   ├── CMakeDetermineCompilerABI_C.bin
│       │   │   │   ├── CMakeDetermineCompilerABI_CXX.bin
│       │   │   │   ├── CMakeSystem.cmake
│       │   │   │   ├── CompilerIdC
│       │   │   │   │   ├── CMakeCCompilerId.c
│       │   │   │   │   ├── a.out
│       │   │   │   │   └── tmp
│       │   │   │   └── CompilerIdCXX
│       │   │   │       ├── CMakeCXXCompilerId.cpp
│       │   │   │       ├── a.out
│       │   │   │       └── tmp
│       │   │   ├── CMakeDirectoryInformation.cmake
│       │   │   ├── CMakeOutput.log
│       │   │   ├── CMakeTmp
│       │   │   ├── Makefile.cmake
│       │   │   ├── Makefile2
│       │   │   ├── TargetDirectories.txt
│       │   │   ├── cmake.check_cache
│       │   │   ├── progress.marks
│       │   │   └── server.dir
│       │   │       ├── DependInfo.cmake
│       │   │       ├── build.make
│       │   │       ├── cmake_clean.cmake
│       │   │       ├── compiler_depend.make
│       │   │       ├── compiler_depend.ts
│       │   │       ├── depend.make
│       │   │       ├── flags.make
│       │   │       ├── link.txt
│       │   │       └── progress.make
│       │   ├── Makefile
│       │   └── cmake_install.cmake
│       ├── mserver
│       └── mserver.cc
├── include
│   ├── ChatServer.hpp
│   ├── ChatService.hpp
│   ├── Common.h
│   ├── db
│   │   └── mysql.hpp
│   ├── json
│   │   └── json.hpp
│   ├── model
│   │   ├── FriendModel.hxx
│   │   ├── GroupModel.hxx
│   │   ├── OfflineMsgModel.hxx
│   │   └── UserModel.hxx
│   └── muduo
│       ├── base
│       │   ├── AsyncLogging.h
│       │   ├── Atomic.h
│       │   ├── BlockingQueue.h
│       │   ├── BoundedBlockingQueue.h
│       │   ├── Condition.h
│       │   ├── CountDownLatch.h
│       │   ├── CurrentThread.h
│       │   ├── Date.h
│       │   ├── Exception.h
│       │   ├── FileUtil.h
│       │   ├── GzipFile.h
│       │   ├── LogFile.h
│       │   ├── LogStream.h
│       │   ├── Logging.h
│       │   ├── Mutex.h
│       │   ├── ProcessInfo.h
│       │   ├── Singleton.h
│       │   ├── StringPiece.h
│       │   ├── Thread.h
│       │   ├── ThreadLocal.h
│       │   ├── ThreadLocalSingleton.h
│       │   ├── ThreadPool.h
│       │   ├── TimeZone.h
│       │   ├── Timestamp.h
│       │   ├── Types.h
│       │   ├── WeakCallback.h
│       │   ├── copyable.h
│       │   └── noncopyable.h
│       └── net
│           ├── Buffer.h
│           ├── Callbacks.h
│           ├── Channel.h
│           ├── Endian.h
│           ├── EventLoop.h
│           ├── EventLoopThread.h
│           ├── EventLoopThreadPool.h
│           ├── InetAddress.h
│           ├── TcpClient.h
│           ├── TcpConnection.h
│           ├── TcpServer.h
│           ├── TimerId.h
│           ├── http
│           │   ├── HttpContext.h
│           │   ├── HttpRequest.h
│           │   ├── HttpResponse.h
│           │   └── HttpServer.h
│           └── inspect
│               └── Inspector.h
├── libs
│   ├── libmysqlclient.a
│   ├── libmysqlclient.so
│   ├── libpthread.a
│   ├── libpthread.so.0
│   └── muduo
│       ├── libmuduo_base.a
│       ├── libmuduo_http.a
│       ├── libmuduo_inspect.a
│       └── libmuduo_net.a
└── src
    ├── Application.cc
    ├── CMakeLists.txt
    ├── ChatServer.cpp
    ├── ChatService.cpp
    └── model
        ├── FriendModel.cxx
        ├── GroupModel.cxx
        ├── OfflineMsgModel.cxx
        └── UserModel.cxx

40 directories, 166 files


---  

# nginx的安装
```shell
$ sudo apt install curl gnupg2 ca-certificates lsb-release ubuntu-keyring 
$ curl https://nginx.org/keys/nginx_signing.key | gpg --dearmor | sudo tee /usr/share/keyrings/nginx-archive-keyring.gpg >/dev/null
$ gpg --dry-run --quiet --no-keyring --import --import-options import-show /usr/share/keyrings/nginx-archive-keyring.gpg
$ echo "deb [signed-by=/usr/share/keyrings/nginx-archive-keyring.gpg] http://nginx.org/packages/ubuntu `lsb_release -cs` nginx" | sudo tee /etc/apt/sources.list.d/nginx.list
$ echo -e "Package: *\nPin: origin nginx.org\nPin: release o=nginx\nPin-Priority: 900\n" | sudo tee /etc/apt/preferences.d/99nginx
$ sudo apt update
$ sudo apt install nginx
```

```plaintext
nginx version: nginx/1.28.0
built by gcc 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04) 
built with OpenSSL 3.0.2 15 Mar 2022
TLS SNI support enabled
configure arguments: --prefix=/etc/nginx --sbin-path=/usr/sbin/nginx --modules-path=/usr/lib/nginx/modules --conf-path=/etc/nginx/nginx.conf --error-log-path=/var/log/nginx/error.log --http-log-path=/var/log/nginx/access.log --pid-path=/run/nginx.pid --lock-path=/run/nginx.lock --http-client-body-temp-path=/var/cache/nginx/client_temp --http-proxy-temp-path=/var/cache/nginx/proxy_temp --http-fastcgi-temp-path=/var/cache/nginx/fastcgi_temp --http-uwsgi-temp-path=/var/cache/nginx/uwsgi_temp --http-scgi-temp-path=/var/cache/nginx/scgi_temp --user=nginx --group=nginx --with-compat --with-file-aio --with-threads --with-http_addition_module --with-http_auth_request_module --with-http_dav_module --with-http_flv_module --with-http_gunzip_module --with-http_gzip_static_module --with-http_mp4_module --with-http_random_index_module --with-http_realip_module --with-http_secure_link_module --with-http_slice_module --with-http_ssl_module --with-http_stub_status_module --with-http_sub_module --with-http_v2_module --with-http_v3_module --with-mail --with-mail_ssl_module --with-stream --with-stream_realip_module --with-stream_ssl_module --with-stream_ssl_preread_module --with-cc-opt='-g -O2 -ffile-prefix-map=/home/builder/debuild/nginx-1.28.0/debian/debuild-base/nginx-1.28.0=. -flto=auto -ffat-lto-objects -flto=auto -ffat-lto-objects -fstack-protector-strong -Wformat -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -fPIC' --with-ld-opt='-Wl,-Bsymbolic-functions -flto=auto -ffat-lto-objects -flto=auto -Wl,-z,relro -Wl,-z,now -Wl,--as-needed -pie'
```

<!-- nginx tcp loadbalance configure -->
stream {
    upstream ChatServer {
        server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
        server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
    }

    server {
        proxy_connect_timeout 1s;
        #proxy_timeout 3s;
        listen 2396;
        proxy_pass ChatServer;
        tcp_nodelay on;
    }
}

sudo systemctl restart nginx