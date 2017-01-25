cmake_minimum_required(VERSION 3.6)
project(pipelinecpp)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(test)

set(SOURCE_FILES library.cpp library.h ringbuffer.hh)
add_library(pipelinecpp ${SOURCE_FILES})