cmake_minimum_required(VERSION 3.22.1)

project(poll_echo_server_partial_writes)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(poll_echo_server
    main.cpp)
