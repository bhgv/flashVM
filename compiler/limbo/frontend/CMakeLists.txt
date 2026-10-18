# Standalone C99 syntax frontend; Coco/R is an explicit external build tool.
cmake_minimum_required(VERSION 3.16)
project(limbo_frontend VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(COCOR_ROOT "" CACHE PATH "Root of bhgv/coco-r-C, containing frames/")
set(COCOR_EXECUTABLE "" CACHE FILEPATH "Built C Coco/R executable")
if(NOT EXISTS "${COCOR_EXECUTABLE}" OR NOT EXISTS "${COCOR_ROOT}/frames/scan_c.frm")
  message(FATAL_ERROR "Set COCOR_EXECUTABLE and COCOR_ROOT to the external C generator")
endif()
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${generated}")
add_custom_command(
  OUTPUT
    "${generated}/limbop.c"
    "${generated}/limbos.c"
    "${generated}/limbop.h"
    "${generated}/limbos.h"
    "${generated}/limboc.h"
    "${generated}/limboe.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/limbo.atg"
    "${generated}/limbo.atg"
  COMMAND ${CMAKE_COMMAND} -E env "CRFRAMES=${COCOR_ROOT}/frames" "${COCOR_EXECUTABLE}" limbo.atg
  WORKING_DIRECTORY "${generated}"
  DEPENDS
    limbo.atg
    "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm"
    "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm"
    "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
add_executable(limbo_parse main.c "${generated}/limbop.c"
    "${generated}/limbos.c")
target_include_directories(limbo_parse PRIVATE "${generated}")
# Each executable owns its generator globals; do not link different parsers together.
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME limbo_syntax
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py"
      $<TARGET_FILE:limbo_parse>)
endif()
