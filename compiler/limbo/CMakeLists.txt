# Standalone language project; frontend owns all required source and tooling.
cmake_minimum_required(VERSION 3.10)
project(limbo_compiler VERSION 0.9.1 LANGUAGES C)
include(CTest)
add_subdirectory(frontend)
