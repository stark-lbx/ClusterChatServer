// clang-format off
#ifndef MYSQL_COMPLECTION_H
#define MYSQL_COMPLECTION_H

#include <string>
#include <mysql/mysql.h>
#include <cstdio>

namespace{
static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chatserver";
}

namespace chat
{
namespace db
{

class MySQL
{
public:
    MySQL(){ conn_ = ::mysql_init(nullptr); }
    
    ~MySQL(){ if(conn_) mysql_close(conn_); }

    bool connect() {
        MYSQL *p = ::mysql_real_connect(conn_, 
                                        server.c_str(),
                                        user.c_str(),
                                        password.c_str(),
                                        dbname.c_str(),
                                        3306, nullptr, 0);
        if(p != nullptr)
        {
            // 设置中文信息(C/C++代码默认的编码是ASCII码、如果不设置，从mysql拉下来的中文显示'?')
            ::mysql_query(conn_, "set names gbk");
            ::puts("[db::mysql::connect] success!");
        }
        else 
        {
            ::puts("[db::mysql::connect] failed!");
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

private:
    MYSQL* conn_;
};

} // namespace db
} // namespace chat

#endif // MYSQL_COMPLECTION_H