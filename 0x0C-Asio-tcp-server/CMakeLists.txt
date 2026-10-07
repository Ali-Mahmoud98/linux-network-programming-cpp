cmake_minimum_required(VERSION 3.22.1)

project(asio_tcp_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asio-tcp-server1 main.cpp)
add_executable(asio-tcp-server2 main2.cpp)

foreach(target asio-tcp-server1 asio-tcp-server2)
    target_include_directories(${target} PRIVATE
        /usr/include
    )
endforeach()
