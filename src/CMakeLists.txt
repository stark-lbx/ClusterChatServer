add_executable(ChatServer
    UserModel.cxx
    OfflineMsgModel.cxx
    FriendModel.cxx
    ChatService.cpp
    ChatServer.cpp
    Application.cc
)

target_include_directories(ChatServer PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
#   ${PROJECT_SOURCE_DIR}/include/third
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