#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>

// json 序列化1
std::string example_01()
{
    json js;
    // add vector
    js["id"] = {1, 2, 3, 4, 5, 6};

    // add key-value
    js["msg_type"] = "2";

    // add object
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["li si"] = "hello china";
    /* or */ // js["msg"] = {{"zhang san", "hello world"}, {"li si", "hello china"}};

    // std::cout << js << std::endl;

    return js.dump();// mean: toString();
}

// json 序列化2
std::string example_02()
{
    // 容器序列化
    json js;

    // 直接序列化一个vector容器
    std::vector<int> vec = {1, 2, 3, 4, 5, 6};
    js["list"] = vec;

    std::map<int, std::string> mp;
    mp.insert({1, "泰山"});
    mp.insert({2, "华山"});
    js["path"] = mp;

    return js.dump();
}

// json 反序列化
void example_03()
{
    // 从json反序列化到容器中

    std::string recvBuf = example_02();
    json jsBuf = json::parse(recvBuf);
    auto arr = jsBuf["list"];
    std::cout << arr[0] << std::endl;

    // auto msg = jsBuf["msg"];
    // std::cout << msg["zhang san"] << std::endl;
    // std::cout << msg["li si"] << std::endl;


    // if(jsBuf["id"].is_array())
    //     std::vector<int> ids = jsBuf["id"].array();
    // if(jsBuf["msg_type"].is_string())
    //     std::string msgType = jsBuf["msg_type"].get_to<std::string>();

    std::vector<int> vec = jsBuf["list"];
    for(const auto& item: vec)
    {
        std::cout << item <<" ";
    }

    std::cout << std::endl;

    std::map<int, std::string> mymap = jsBuf["path"];
    for(const auto& p: mymap)
    {
        std::cout <<  p.second << "->";
    }

    std::cout << "\b\b  \n"; // 用两个退格符将光标回退，使用空格符覆盖，之后换行
}

int main()
{
    // std::cout << example_01();
    // std::cout << example_02();
    example_03();

    return 0;
}