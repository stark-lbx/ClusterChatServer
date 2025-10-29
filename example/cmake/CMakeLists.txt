cmake_minimum_required(VERSION 3.0)
project(main)

# 编译选项
# 对应 -g -o
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置头文件搜索路径
# 对应 -I/usr/local/include
# include_directories()

# 配置库文件搜索路径
# 对应 -L/usr/local/lib/muduo
link_directories(/usr/local/lib/muduo)

# 设置需要编译的源文件列表
set(SRC_LIST 
 mserver.cc
 )
# 代替方案二：(将当前文件下的所有源文件设置为SRC_LIST)
# aux_source_directory(*.cpp *.cc SRC_LIST)
# 代替方案三：全局（不推荐）
# file(GLOB SRC_LIST "*.cc")

# 设置可执行文件的最终存储路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 生成可执行文件
add_executable(server ${SRC_LIST})
# 需要链接的库文件
# 对应 -lmuduo_net -lmuduo_base -lpthread
target_link_libraries(server muduo_net muduo_base pthread)