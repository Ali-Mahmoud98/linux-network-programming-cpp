cmake_minimum_required(VERSION 3.22.1)

project(IO_multiplexing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(epoll-lab1)
add_subdirectory(poll-echo-server-partial-writes)
add_subdirectory(epoll-lab2-stdin)
add_subdirectory(epoll-lab3-tcp)
add_subdirectory(epoll-lab4-et)
