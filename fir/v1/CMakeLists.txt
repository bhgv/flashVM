# Independent C99 component. Detached copies need only the root MIT LICENSE.
cmake_minimum_required(VERSION 3.16)
project(fir1 VERSION 0.3.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(COCOR_ROOT "" CACHE PATH "External bhgv/coco-r-C root")
set(COCOR_EXECUTABLE "" CACHE FILEPATH "External C Coco/R executable")
if(NOT EXISTS "${COCOR_EXECUTABLE}" OR NOT EXISTS "${COCOR_ROOT}/frames/scan_c.frm")
  message(FATAL_ERROR "Set COCOR_ROOT and COCOR_EXECUTABLE to bhgv/coco-r-C")
endif()
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${generated}")
add_custom_command(OUTPUT "${generated}/fir1p.c" "${generated}/fir1s.c"
  "${generated}/fir1p.h" "${generated}/fir1s.h" "${generated}/fir1c.h" "${generated}/fir1e.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/compiler/fir1.atg" "${generated}/fir1.atg"
  COMMAND ${CMAKE_COMMAND} -E env "CRFRAMES=${COCOR_ROOT}/frames" "${COCOR_EXECUTABLE}" fir1.atg
  WORKING_DIRECTORY "${generated}"
  DEPENDS compiler/fir1.atg "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm" "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm" "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
# Generated legacy helpers retain upstream warning policy; first-party code
# still builds with strict warnings. No upstream files are copied into Git.
add_library(fir1_generated OBJECT "${generated}/fir1p.c" "${generated}/fir1s.c")
target_compile_definitions(fir1_generated PUBLIC CUSTOM_BUFF)
target_include_directories(fir1_generated PRIVATE include src)
target_include_directories(fir1_generated PUBLIC "${generated}")
add_library(fir1_ir src/ir.c src/input.c src/builder.c optimiser/peephole.c optimiser/effects.c compiler/object.c)
target_link_libraries(fir1_ir PUBLIC fir1_generated)
target_include_directories(fir1_ir PUBLIC include)
add_executable(fir1 src/main.c)
target_link_libraries(fir1 PRIVATE fir1_ir)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(fir1_ir PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(fir1 PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
# The source grammar has its own executable: never link its Parse/S_Get globals
# with those of fir1. Both grammars use the same explicit external generator.
add_custom_command(OUTPUT "${generated}/src1p.c" "${generated}/src1s.c"
  "${generated}/src1p.h" "${generated}/src1s.h" "${generated}/src1c.h" "${generated}/src1e.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/compiler/src1.atg" "${generated}/src1.atg"
  COMMAND ${CMAKE_COMMAND} -E env "CRFRAMES=${COCOR_ROOT}/frames" "${COCOR_EXECUTABLE}" src1.atg
  WORKING_DIRECTORY "${generated}"
  DEPENDS compiler/src1.atg "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm" "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm" "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
add_library(fir1_source_generated OBJECT "${generated}/src1p.c" "${generated}/src1s.c")
target_compile_definitions(fir1_source_generated PUBLIC CUSTOM_BUFF)
target_include_directories(fir1_source_generated PUBLIC "${generated}")
add_executable(fir1_source compiler/source.c compiler/lower.c src/input.c)
target_include_directories(fir1_source_generated PRIVATE compiler)
target_link_libraries(fir1_source PRIVATE fir1_source_generated)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(fir1_source PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME fir1_acceptance COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_source>)
endif()
