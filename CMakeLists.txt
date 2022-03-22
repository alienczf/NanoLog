cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(Compiler)
project(NanoLog VERSION 0.0.1 DESCRIPTION "Nanolog is an extremely performant nanosecond scale logging system for C++ that exposes a simple printf-like API.")
include(Flags)
include(Utils)

add_subdirectory(runtime)
add_subdirectory(logDecompressor)
add_subdirectory(perf)
add_subdirectory(integrationTest)
add_subdirectory(benchmarks)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/_run" CACHE PATH "default install path" FORCE)
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/../lib64")
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
