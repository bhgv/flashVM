# Copyright (c) 2026 bhgv
# EN: Independent native carrier layer, no VM/graphics or wire-codec dependency.
# RU: Независимый native carrier layer без VM/graphics и wire-codec dependency.
cmake_minimum_required(VERSION 3.10)
project(remote_phy VERSION 0.1 LANGUAGES C)
add_library(remote_phy STATIC phy.c select.c)
target_include_directories(remote_phy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(remote_phy PROPERTIES C_STANDARD 99)
target_compile_options(remote_phy PRIVATE -Wall -Wextra -Wpedantic)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(remote_phy PRIVATE drivers/posix/fd.c)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    add_executable(remote-phy-core tests/core.c)
    target_link_libraries(remote-phy-core PRIVATE remote_phy)
    add_test(NAME remote-phy-core COMMAND remote-phy-core)
endif()
