# Standalone language project; frontend owns all required source and tooling.
cmake_minimum_required(VERSION 3.10)
project(as2_compiler VERSION 0.4.0 LANGUAGES C)
include(CTest)
add_subdirectory(frontend)
