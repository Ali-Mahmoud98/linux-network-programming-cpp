cmake_minimum_required(VERSION 3.16)

project(epoll_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    epoll_server

    main.cpp

    socket/TCPSocket.cpp

    server/ClientConnection.cpp
    server/EpollServer.cpp
)

target_include_directories(
    epoll_server
    PRIVATE
        interface
        socket
        server
)
