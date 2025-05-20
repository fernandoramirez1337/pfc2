# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(sand_bench C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(sand STATIC SAND.c)
add_executable(sand_bench main.c)

target_link_libraries(sand_bench PRIVATE sand)
target_include_directories(sand_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})