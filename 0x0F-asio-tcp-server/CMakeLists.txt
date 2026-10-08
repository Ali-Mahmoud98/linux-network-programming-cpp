cmake_minimum_required(VERSION 3.22.1)

project(asio_tcp_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asio-tcp-server
    main.cpp
    connection/Connection.cpp
)

target_include_directories(asio-tcp-server 
PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/connection
)
