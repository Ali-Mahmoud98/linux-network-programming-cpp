cmake_minimum_required(VERSION 3.22.1)

project(cpp_socket 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# create the socket library
add_library(socket socket/Socket.cpp)
# specify include directories for the socket library
target_include_directories(socket
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/interface
        ${CMAKE_CURRENT_SOURCE_DIR}/socket
)

# add the server executable
add_executable(server server/main.cpp)
# link the server executable with the socket library
target_link_libraries(server PRIVATE socket)

# add the client executable
add_executable(client client/main.cpp)
# link the client executable with the socket library
target_link_libraries(client PRIVATE socket)