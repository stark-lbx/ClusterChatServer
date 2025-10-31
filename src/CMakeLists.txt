add_executable(ChatServer
    model/UserModel.cxx
    model/OfflineMsgModel.cxx
    model/FriendModel.cxx
    model/GroupModel.cxx
    ChatService.cpp
    ChatServer.cpp
    Application.cc
)

target_link_directories(ChatServer PRIVATE 
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/muduo
    ${PROJECT_SOURCE_DIR}/libs/hiredis
)

target_link_libraries(ChatServer PRIVATE 
    muduo_net 
    muduo_base
    mysqlclient 
    hiredis
    pthread
)