// clang-format off

#include "GroupModel.hxx"
#include "db/mysql.hpp"

bool chat::model::GroupModel::create(Group & group)
{
    char sql[1024] = {0};
    ::sprintf(sql, "insert into AllGroup(groupname, groupdesc) values('%s', '%s')", 
        group.groupname().c_str(), group.description().c_str());

    db::MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            group.setGroupid(::mysql_insert_id(mysql.connection()));
            return true;
        }

    }
    return false;
}

bool chat::model::GroupModel::join(int userid, int groupid, const std::string & role)
{
    char sql[1024] = {0};
    ::sprintf(sql, "insert into GroupUser(groupid, userid, grouprole) values(%d, %d, '%s')", userid, groupid, role.c_str());

    db::MySQL mysql;
    if(mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}

std::vector<chat::Group> chat::model::GroupModel::query(int userid)
{
    char sql[1024] = {0};
    ::sprintf(sql, "select a.groupid, a.groupname, a.groupdesc from AllGroup a \
        join GroupUser b on a.groupid=b.groupid where b.userid=%d", userid);

    std::vector<chat::Group> results;

    db::MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = ::mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setGroupid(::atoi(row[0]));
                group.setGroupname(row[1]);
                group.setDescription(row[2]);
                results.emplace_back(std::move(group));
            }
            ::mysql_free_result(res);
        }
    }
    for(auto& group: results)
    {
        ::sprintf(sql, "select u.userid, u.username, u.state, g.grouprole from User u \
            inner join GroupUser g on g.userid=u.userid where g.groupid=%d", group.groupid());

        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = ::mysql_fetch_row(res)) != nullptr)
            {
                GroupUser guser;
                guser.setUserid(::atoi(row[0]));
                guser.setUsername(row[1]);
                guser.setState(row[2]);
                guser.setRole(row[3]);
                group.members().push_back(guser);
            }
            ::mysql_free_result(res);
        }
    }

    return results;
}

std::vector<int> chat::model::GroupModel::query(int userid, int groupid)
{
    char sql[1024] = {0};
    ::sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d", groupid, userid);

    std::vector<int> results;

    db::MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = ::mysql_fetch_row(res)) != nullptr)
            {
                results.emplace_back(::atoi(row[0]));
            }
            ::mysql_free_result(res);
        }
    }
    return results;
}
