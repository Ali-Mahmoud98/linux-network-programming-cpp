cmake_minimum_required(VERSION 3.22.1)

project(epoll_partial_write_lab3_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(epoll-echo-server main.cpp)