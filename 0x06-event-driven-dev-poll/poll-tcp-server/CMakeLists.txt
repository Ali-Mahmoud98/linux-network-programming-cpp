cmake_minimum_required(VERSION 3.22.1)

project(poll_tcp_server
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(poll_tcp_server_lab
    main.cpp
)
