cmake_minimum_required(VERSION 3.22.1)

project(asio_learning)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asio_hello main.cpp)
add_executable(asio_run main2.cpp)

foreach(target asio_hello asio_run)
    target_include_directories(${target} PRIVATE
        /usr/include
    )
endforeach()
