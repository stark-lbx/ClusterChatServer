# 设置proto文件
set(PROTO_FILES
    base.proto
    friend.proto 
    message.proto 
    notify.proto 
    user.proto
    group.proto
)

set(PROTO_SRCS "")
foreach(PROTO_FILE ${PROTO_FILES})
    string(REPLACE ".proto" ".pb.cc" PROTO_CC ${PROTO_FILE})
    string(REPLACE ".proto" ".pb.h" PROTO_HH ${PROTO_FILE})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/proto/${PROTO_CC})
        add_custom_command(
            PRE_BUILD
            COMMAND protoc
            ARGS --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto -I ${PROJECT_SOURCE_DIR}/proto ${PROJECT_SOURCE_DIR}/proto/${PROTO_FILE}
            DEPENDS ${PROJECT_SOURCE_DIR}/proto/${PROTO_FILE}
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/proto/${PROTO_CC}
            COMMENT "生成protobuf框架代码文件:" ${CMAKE_CURRENT_SOURCE_DIR}/proto/${PROTO_CC}
        )
    endif()
    list(APPEND PROTO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/proto/${PROTO_CC})
endforeach()

# 创建可执行文件
add_executable(ChatServer
    model/UserModel.cxx
    model/OfflineMsgModel.cxx
    model/FriendModel.cxx
    model/GroupModel.cxx
    ChatService.cpp
    ChatServer.cpp
    Application.cc
    ${PROTO_SRCS}
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
    protobuf
)