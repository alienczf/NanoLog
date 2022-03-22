cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")
project(NanoLog VERSION 0.0.1 DESCRIPTION "Nanolog is an extremely performant nanosecond scale logging system for C++ that exposes a simple printf-like API.")
include(Flags)
include(Utils)

add_subdirectory(core)
add_subdirectory(logDecompressor)
add_subdirectory(perf)
add_subdirectory(integrationTest)
add_subdirectory(benchmarks)
