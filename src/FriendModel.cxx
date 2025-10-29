// clang-format off
#include "FriendModel.hxx"
#include "db/mysql.hpp"

bool chat::model::FriendModel::insert(int userid, int friendid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    ::sprintf(sql, "insert into Friend values(%d, %d)", userid, friendid);

    chat::db::MySQL mysql;
    if(mysql.connect())
    {
        return mysql.update(sql);
    }

    return false;
}

std::vector<chat::User> chat::model::FriendModel::query(int userid)
{
    std::vector<chat::User> results;

    // 1. 组装sql语句
    char sql[1024] = {0};
    // const char* sql_format = R"(
    //     select * 
    //     from User 
    //     where userid in (
    //         select friendid 
    //         from Friend 
    //         where userid = %d
    //     )
    // )"; // 子查询

    // const char* sql_format = R"(
    //     select u.userid, u.username, u.state 
    //     from User u
    //     inner join 
    //         Friend f
    //         on f.friendid = u.userid
    //     where f.userid = %d
    // )"; // 连接查询(内连接)

    const char* sql_format = R"(
        -- 查找用户作为userid的好友（主动添加的）
        select u.userid, u.username, u.state
        from User u
        inner join Friend f on u.userid = f.friendid
        where f.userid = %d

        UNION

        -- 查找用户作为friendid的好友（被添加的）
        select u.userid, u.username, u.state  
        from User u
        inner join Friend f on u.userid = f.userid
        where f.friendid = %d
    )";

    ::sprintf(sql, sql_format, userid, userid);

    chat::db::MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = ::mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setUserid(::atoi(row[0]));
                user.setUsername(row[1]);
                user.setState(row[2]);
                results.emplace_back(user);
            }
            mysql_free_result(res);
        }

    }

    return results;

}
