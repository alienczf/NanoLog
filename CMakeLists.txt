cmake_minimum_required(VERSION 3.10)
# The additional / is important to remove the last character from the path.
# Note that it does not matter if the OS uses / or \, because we are only
# saving the path size.
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")


list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")
project(NanoLog VERSION 0.0.1 DESCRIPTION "Nanolog is an extremely performant nanosecond scale logging system for C++ that exposes a simple printf-like API.")
option(BUILD_DEV "build dev targets" OFF)
include(Flags)
include(Utils)

add_subdirectory(core)
add_subdirectory(logDecompressor)
if (BUILD_DEV)
  add_subdirectory(perf)
  add_subdirectory(integrationTest)
  add_subdirectory(benchmarks)
endif()
