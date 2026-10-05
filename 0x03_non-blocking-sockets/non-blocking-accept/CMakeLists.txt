cmake_minimum_required(VERSION 3.22.1)

project(non_blocking_accept
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(non_blocking_accept_lab
    main.cpp
)

add_executable(non_blocking_accept_lab2
    main2.cpp
)
