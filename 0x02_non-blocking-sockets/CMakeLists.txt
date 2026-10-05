cmake_minimum_required(VERSION 3.22.1)

project(non_blocking_sockets 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server main.cpp)
add_executable(stdin-non-block stdin-non-blocking.cpp)
