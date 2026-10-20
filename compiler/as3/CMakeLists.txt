# Standalone language project; frontend owns all required source and tooling.
cmake_minimum_required(VERSION 3.16)
project(as3_compiler VERSION 0.3.2 LANGUAGES C)
include(CTest)
add_subdirectory(frontend)
