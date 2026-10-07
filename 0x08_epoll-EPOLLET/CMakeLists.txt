cmake_minimum_required(VERSION 3.22.1)

project(epoll_EPOLLET LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(epoll-lab1)
add_subdirectory(epoll-partial-write-lab1)
add_subdirectory(epoll-partial-write-lab2)
add_subdirectory(epoll-partial-write-lab3)
add_subdirectory(epoll-partial-write-lab3-1)
add_subdirectory(epoll-partial-write-lab4)

