// clang-format off
#ifndef USER_H
#define USER_H

#include <string>
#include <memory>

namespace chat
{

class User
{
public:
    User(std::string state = "offline") : state_(state) {}
    User(int userid, std::string username, std::string password, std::string state = "offline")
        : userid_(userid), username_(username), password_(password), state_(state){}
    
    void setUserid(int userid) { userid_ = userid;}
    void setUsername(std::string username) {username_ = username;}
    void setPassword(std::string password) {password_ = password;}
    void setState(std::string state) {state_ = state;}

    const int& userid() const {return userid_;}
    const std::string& username() const {return username_;}
    const std::string& password() const {return password_;}
    const std::string& state() const {return state_;}
    
private:
            int userid_;            //auto_increment primary key
    std::string username_;          //not null unique
    std::string password_;          //not null
    std::string state_;             //default "offline"
};


namespace model
{

class UserModel
{
public:
    // User表的增加方法
    bool insert(User& user);
    // 跟新用户信息
    bool update(const User& user);
    // 根据id查询用户
    std::unique_ptr<User> query(int id);
};

} // namespace model
} // namespace chat


#endif