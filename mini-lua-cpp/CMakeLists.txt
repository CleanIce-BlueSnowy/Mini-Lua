cmake_minimum_required(VERSION 4.2)

project(mini-lua-cpp)

set(CMAKE_CXX_STANDARD 26)

add_executable(mini-lua-cpp src/main.cpp)
target_compile_options(mini-lua-cpp PUBLIC -O2 -Wall -Wextra)
