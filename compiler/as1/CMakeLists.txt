# Standalone language project; frontend owns all required source and tooling.
cmake_minimum_required(VERSION 3.16)
project(as1_compiler VERSION 0.1.0 LANGUAGES C)
include(CTest)
add_subdirectory(frontend)
