// clang-format off
#include "OfflineMsgModel.hxx"
#include "middle/mysql.hpp"
#include <iostream>

bool chat::model::OfflineMsgModel::insert(int userid, const std::string & msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage values(%d, '%s')", userid, msg.c_str());

    auto mysql = db::ConnectionPool::instance()->getConnection();
    if (mysql)
    {
        return mysql->update(sql);
    }

    return false;
}

bool chat::model::OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid = %d", userid);

    auto mysql = db::ConnectionPool::instance()->getConnection();
    if (mysql)
    {
        return mysql->update(sql);
    }
    return false;
}

std::vector<std::string> chat::model::OfflineMsgModel::query(int userid)
{
    std::vector<std::string> results;

    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid = %d", userid);

    auto mysql = db::ConnectionPool::instance()->getConnection();
    if (mysql)
    {
        MYSQL_RES* res = mysql->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                results.emplace_back(row[0]);
            }
            mysql_free_result(res);
        }
    }

    return std::move(results); // 不写默认优化为move
}
