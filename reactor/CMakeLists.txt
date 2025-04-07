#获取当前目录下的所有源文件项目
file(GLOB REACTOR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*cpp)


# 创建共享库 util_lib
add_library(reactor_lib SHARED ${REACTOR_FILE})