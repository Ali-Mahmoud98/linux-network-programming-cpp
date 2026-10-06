cmake_minimum_required(VERSION 3.22.1)
project(event_driven_dev_poll)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(select-tcp-server)
add_subdirectory(poll-tcp-server)
