# Standalone C99 syntax frontend; Coco/R is an explicit external build tool.
cmake_minimum_required(VERSION 3.10)
project(firsyn_frontend VERSION 0.1.3 LANGUAGES C)
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
    "${generated}/firsynp.c"
    "${generated}/firsyns.c"
    "${generated}/firsynp.h"
    "${generated}/firsyns.h"
    "${generated}/firsync.h"
    "${generated}/firsyne.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/firsyn.atg"
    "${generated}/firsyn.atg"
  COMMAND ${CMAKE_COMMAND} "-DCOCOR_EXECUTABLE=${COCOR_EXECUTABLE}"
    "-DCOCOR_ROOT=${COCOR_ROOT}" -DGRAMMAR=firsyn
    -P "${CMAKE_CURRENT_SOURCE_DIR}/run_cocor.cmake"
  WORKING_DIRECTORY "${generated}"
  DEPENDS
    run_cocor.cmake
    firsyn.atg
    "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm"
    "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm"
    "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
set_source_files_properties("${generated}/firsynp.c" "${generated}/firsyns.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(firsyn_frontend api.c parser.c scanner.c "${generated}/firsynp.c" "${generated}/firsyns.c")
target_include_directories(firsyn_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(firsyn_frontend PRIVATE "${generated}")
target_compile_definitions(firsyn_frontend PRIVATE CUSTOM_BUFF)
# Prefix every exported legacy scanner/parser global; no generator-frame edits are required.
set(parser_symbols
  Custom_Error GenError Parse SemError Successful SynError
  S_Col S_CurrLine S_CurrentCh S_Get S_GetName S_GetString S_Len S_Line S_LineStart
  S_NextCol S_NextLen S_NextLine S_NextPos S_Pos S_Reset S_src)
foreach(symbol IN LISTS parser_symbols)
  target_compile_definitions(firsyn_frontend PRIVATE "${symbol}=firsyn_${symbol}")
endforeach()

add_executable(firsyn_parse main.c)
target_link_libraries(firsyn_parse PRIVATE firsyn_frontend)
# Each library owns namespaced globals; serialize calls to the same parser.
include(CTest)
if(BUILD_TESTING)
  # FindPython3 starts at CMake 3.12; retain standalone 3.10 builds.
  if(CMAKE_VERSION VERSION_LESS "3.12")
    find_package(PythonInterp 3 REQUIRED)
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    set(Python3_Interpreter_FOUND "${PYTHONINTERP_FOUND}")
  else()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
  endif()
  add_test(NAME fir_syntax COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py $<TARGET_FILE:firsyn_parse>)
  add_executable(firsyn_embed tests/embed.c)
  target_link_libraries(firsyn_embed PRIVATE firsyn_frontend)
  add_test(NAME fir_syntax_embedding COMMAND firsyn_embed)
endif()

if(BUILD_TESTING AND TARGET fir1)
  add_test(NAME fir_syntax_backend_boundary COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/backend_check.py $<TARGET_FILE:fir1>)
endif()

# Exercise the host generator failure boundary, not the target CLI.
if(BUILD_TESTING)
  add_test(NAME firsyn_generator_failure COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/generator_check.py ${CMAKE_COMMAND}
    ${CMAKE_CURRENT_SOURCE_DIR}/run_cocor.cmake ${COCOR_EXECUTABLE} ${COCOR_ROOT})
endif()
