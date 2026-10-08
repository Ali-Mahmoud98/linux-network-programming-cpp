cmake_minimum_required(VERSION 3.22.1)

project(asio_tcp_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asio_tcp_echo_server main.cpp)
