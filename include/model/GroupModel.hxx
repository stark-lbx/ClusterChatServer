// clang-format off
#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include <string>
#include <vector>
#include "UserModel.hxx"

namespace chat
{

// 群组内的用户:
// 多了一个role角色信息，从User类直接继承、复用User的其它信息

class GroupUser : public User
{
public:
    void setRole(const std::string& role) { role_ = role;}
    const std::string& role() const {return role_;}

private:
    std::string role_;
};

class Group
{
public:
    Group()=default;
    Group(int id, const std::string& name, const std::string& desc)
        : groupid_(id), groupname_(name), description_(desc) {}

    void setGroupid(int id){groupid_ = id;}
    void setGroupname(const std::string& name) {groupname_ = name;}
    void setDescription(const std::string& desc) {description_ = desc;}

    int groupid() const {return groupid_;}
    const std::string& groupname() const {return groupname_;}
    const std::string& description() const {return description_;}
    std::vector<chat::GroupUser>& members() {return members_;}

private:
    int groupid_;
    std::string groupname_;
    std::string description_;
    std::vector<chat::GroupUser> members_;
};

namespace model
{
    
class GroupModel
{
public:
    // 创建群组
    bool create(Group& group);
    // 加入群组
    bool join(int userid, int groupid, const std::string& role);
    // 查询用户所在的群组信息
    std::vector<Group> query(int userid);
    // 根据指定的groupid查询群组用户的id列表，除了userid自己；主要用于群聊业务：给群组其它成员群发消息
    std::vector<int> query(int userid, int groupid);
};


} // namespace model
} // namespace chat

#endif // GROUPMODEL_H