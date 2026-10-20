# Standalone language project; frontend owns all required source and tooling.
cmake_minimum_required(VERSION 3.16)
project(limbo_compiler VERSION 0.3.3 LANGUAGES C)
include(CTest)
add_subdirectory(frontend)
