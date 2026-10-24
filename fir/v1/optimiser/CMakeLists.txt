# Local entry point: build the same autonomous v1 toolchain from this folder.
cmake_minimum_required(VERSION 3.10)
project(fir1_local LANGUAGES C)
include(CTest)
add_subdirectory(.. "${CMAKE_CURRENT_BINARY_DIR}/v1")
