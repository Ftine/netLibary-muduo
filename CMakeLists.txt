cmake_minimum_required(VERSION 3.23.5)
project(muduo)
# 添加项目头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/base  ${CMAKE_CURRENT_SOURCE_DIR}/net)

set(CMAKE_CXX_STANDARD 11)

ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(net)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=GBK")
