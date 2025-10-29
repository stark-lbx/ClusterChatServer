// clang-format off
#include "UserModel.hxx"
#include "db/mysql.hpp"
#include <iostream>

bool chat::model::UserModel::insert(User &user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(username, password, state) values('%s', '%s', '%s')",
        user.username().c_str(), user.password().c_str(), user.state().c_str());

    chat::db::MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setUserid(mysql_insert_id(mysql.connection()));
            return true;
        }   
    }

    return false;
}

bool chat::model::UserModel::update(const User & user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update User set username = '%s', password = '%s', state = '%s' where userid = %d",
        user.username().c_str(), user.password().c_str(), user.state().c_str(), user.userid());

    chat::db::MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }   
    }
    return false;
}

std::unique_ptr<chat::User> chat::model::UserModel::query(int id)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    ::sprintf(sql, "select * from User where userid = %d", id);

    chat::db::MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row = ::mysql_fetch_row(res);
            if(row != nullptr)
            {
                auto user = std::make_unique<User>();
                user->setUserid(::atoi(row[0]));
                user->setUsername(row[1]);
                user->setPassword(row[2]);
                user->setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return nullptr;
}

void chat::model::UserModel::offlineAll()
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update User set state = 'offline' where state = 'online'");

    chat::db::MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}
