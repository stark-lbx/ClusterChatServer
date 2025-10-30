// clang-format off
#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "UserModel.hxx"


namespace chat
{

class Friend
{
public:
    Friend() = default;
    Friend(int userid, int friendid)
        : userid_(userid), friendid_(friendid) {}
    
    void setUserid(int userid){userid_ = userid;}
    void setFriendid(int friendid){friendid_ = friendid;}

    int userid() const {return userid_;}
    int friendid() const {return friendid_;}

private:
    int userid_;
    int friendid_;
};

namespace model
{

class FriendModel
{
public:
    // 插入一个好友关系 - userid+friendid
    bool insert(int userid, int friendid);
    bool insert(const Friend& f) {return insert(f.userid(), f.friendid());}

    // 根据userid查询所有的friendUser信息
    std::vector<chat::User> query(int userid); // 多表联查
    // std::vector<int> query(int userid); // 获取id、然后再发起多次 userModel.query() 的mysql请求
};
    
} // namespace model
} // namespace chat


#endif