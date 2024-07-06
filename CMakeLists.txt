cmake_minimum_required(VERSION 3.10)

project(net_test.e)

find_library(LIBRESSL_SSL_LIBRARY ssl)
find_library(LIBRESSL_CRYPTO_LIBRARY crypto)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCES "utils/*.cc" "net/*.cc" "crypto/*.cc" "time_utils.cc" "net_test.cc")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBRESSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRESSL_SSL_LIBRARY} ${LIBRESSL_CRYPTO_LIBRARY})

include_directories(${CMAKE_SOURCE_DIR})
