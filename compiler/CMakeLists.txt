# Optional compiler-only orchestration; each language/frontend builds alone too.
cmake_minimum_required(VERSION 3.16)
project(flash_compilers LANGUAGES C)
include(CTest)
add_subdirectory(as1)
add_subdirectory(as2)
add_subdirectory(limbo)
