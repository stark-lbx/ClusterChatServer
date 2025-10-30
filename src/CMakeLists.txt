add_executable(ChatServer
    model/UserModel.cxx
    model/OfflineMsgModel.cxx
    model/FriendModel.cxx
    model/GroupModel.cxx
    ChatService.cpp
    ChatServer.cpp
    Application.cc
)

target_include_directories(ChatServer PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/model
)

target_link_directories(ChatServer PRIVATE 
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/muduo
)

target_link_libraries(ChatServer PRIVATE 
    muduo_net 
    muduo_base
    mysqlclient 
    pthread
)