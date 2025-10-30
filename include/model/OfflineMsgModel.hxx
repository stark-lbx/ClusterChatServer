// clang-format off
#ifndef OFFLINEMSGMODEL_H
#define OFFLINEMSGMODEL_H

#include <string>
#include <vector>

namespace chat
{

class OfflineMsg{
public:
    OfflineMsg()=default;
    OfflineMsg(int userid, const std::string& message)
        : userid_(userid), message_(message) { }

    void setUserid(int userid){userid_ = userid;}
    void setMessage(const std::string& message) {message_ = message;}

    int userid() const {return userid_;}
    std::string message() const {return message_;}

private:
            int userid_;    // 离线消息所属用户的id
    std::string message_;   // 离线消息的具体内容
};

namespace model
{

class OfflineMsgModel{
public:
    bool insert(int userid, const std::string& msg);
    bool remove(int userid);
    std::vector<std::string> query(int userid);
};


} // namespace model
} // namespace chat


#endif // OFFLINEMSGMODEL_H