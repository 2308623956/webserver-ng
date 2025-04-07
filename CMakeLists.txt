cmake_minimum_required(VERSION 3.10.0)
project(myWebserver VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED True)

#设置头文件目录，供所有子项目使用
include_directories(${CMAKE_SOURCE_DIR}/include)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

#设置全局链接库
set(LIBS
    pthread 
)

add_subdirectory(test)
add_subdirectory(log)
add_subdirectory(util)

