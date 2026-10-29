cmake_minimum_required(VERSION 3.22.1)

project(asio-network-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

option(DEBUG_TRACE "Enable debug tracing" OFF)

# 1. Interfaces and Core library (currently header-only; will include the Framer and Codec in step 03)
add_library(netserver_core INTERFACE)
target_include_directories(netserver_core
INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(DEBUG_TRACE)
    target_compile_definitions(netserver_core INTERFACE DEBUG_TRACE)
endif()

# 2. Transports library (currently TCP, UDP later)
add_library(netserver_transports STATIC
    src/transport/tcp/TcpServer.cpp
    src/transport/tcp/TcpSession.cpp
)

target_link_libraries(netserver_transports
PUBLIC
    netserver_core
    pthread
)

# 3. Server executable
add_executable(asio-tcp-server
    src/main.cpp
)

target_link_libraries(asio-tcp-server
PRIVATE
    netserver_transports
)
