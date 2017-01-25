cmake_minimum_required(VERSION 3.6)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 11)

add_executable(runPipelineCPPTests
        basic_check.cpp pipeline_test.cpp ringbuffer_test.cc)

target_link_libraries(runPipelineCPPTests gtest gtest_main pipelinecpp)