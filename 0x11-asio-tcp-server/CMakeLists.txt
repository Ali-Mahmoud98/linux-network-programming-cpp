cmake_minimum_required(VERSION 3.22.1)

project(asio-tcp-server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asio-tcp-server
    server/Server.cpp
    connection/Connection.cpp
    main.cpp
)

target_include_directories(asio-tcp-server 
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/connection
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)
