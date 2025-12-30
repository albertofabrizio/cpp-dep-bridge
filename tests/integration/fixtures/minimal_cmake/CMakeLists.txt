cmake_minimum_required(VERSION 3.20)
project(minimal LANGUAGES CXX)

add_library(foo STATIC foo.cpp)
target_link_libraries(foo PRIVATE m)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE foo)
