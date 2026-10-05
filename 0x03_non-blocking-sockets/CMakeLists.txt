cmake_minimum_required(VERSION 3.22.1)

project(non_blocking_sockets_labs 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(blocking-socket)
add_subdirectory(non-blocking-accept)
add_subdirectory(blocking-recv)
add_subdirectory(non-blocking-recv)
