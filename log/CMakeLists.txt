#获取当前目录下的所有源文件项目
file(GLOB LOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*cpp)


# 创建共享库 log_lib
add_library(log_lib SHARED ${LOG_FILE})