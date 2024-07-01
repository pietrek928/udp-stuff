cmake_minimum_required(VERSION 3.10)

project(net_test.e)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCES "net/*.cc" "time_utils.cc" "net_test.cc")

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories(${CMAKE_SOURCE_DIR})