cmake_minimum_required(VERSION 3.6)
project(slave_node)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES main.cpp)
add_executable(slave_node ${SOURCE_FILES})