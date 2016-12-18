cmake_minimum_required(VERSION 3.6)
project(master_node)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES main.cpp)
add_executable(master_node ${SOURCE_FILES})