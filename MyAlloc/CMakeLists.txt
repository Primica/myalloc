cmake_minimum_required(VERSION 3.29)
project(untitled C)

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(untitled ${SOURCES})
