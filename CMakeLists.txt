cmake_minimum_required(VERSION 3.10)
project(knock_pattern)

set(CMAKE_CXX_STANDARD 17)

add_executable(knock_pattern src/main.cpp)
target_link_libraries(knock_pattern pigpio)
