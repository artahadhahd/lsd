cmake_minimum_required(VERSION 3.10)
project(lsd)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_executable(lsd
    src/main.cpp
    src/lexer.cpp
    src/lexer.hpp
    src/parser.cpp
    src/parser.hpp
)