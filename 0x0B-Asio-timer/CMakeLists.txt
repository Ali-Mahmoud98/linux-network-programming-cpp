cmake_minimum_required(VERSION 3.22.1)

project(asio_timer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asio-timer1 main.cpp)
add_executable(asio-timer2 main2.cpp)

foreach(target asio-timer1 asio-timer2)
    target_include_directories(${target} PRIVATE
        /usr/include
    )
endforeach()
