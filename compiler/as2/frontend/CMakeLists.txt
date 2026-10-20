# Standalone C99 syntax frontend; Coco/R is an explicit external build tool.
cmake_minimum_required(VERSION 3.16)
project(as2_frontend VERSION 0.2.2 LANGUAGES C)
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
    "${generated}/as2p.c"
    "${generated}/as2s.c"
    "${generated}/as2p.h"
    "${generated}/as2s.h"
    "${generated}/as2c.h"
    "${generated}/as2e.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/as2.atg"
    "${generated}/as2.atg"
  COMMAND ${CMAKE_COMMAND} -E env "CRFRAMES=${COCOR_ROOT}/frames" "${COCOR_EXECUTABLE}" as2.atg
  WORKING_DIRECTORY "${generated}"
  DEPENDS
    as2.atg
    "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm"
    "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm"
    "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
set_source_files_properties("${generated}/as2p.c" "${generated}/as2s.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(as2_frontend api.c parser.c scanner.c "${generated}/as2p.c" "${generated}/as2s.c")
target_include_directories(as2_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(as2_frontend PRIVATE "${generated}")
target_compile_definitions(as2_frontend PRIVATE CUSTOM_BUFF Custom_Error=as2_Custom_Error GenError=as2_GenError Parse=as2_Parse SemError=as2_SemError Successful=as2_Successful SynError=as2_SynError S_Col=as2_S_Col S_CurrLine=as2_S_CurrLine S_CurrentCh=as2_S_CurrentCh S_Get=as2_S_Get S_GetName=as2_S_GetName S_GetString=as2_S_GetString S_Len=as2_S_Len S_Line=as2_S_Line S_LineStart=as2_S_LineStart S_NextCol=as2_S_NextCol S_NextLen=as2_S_NextLen S_NextLine=as2_S_NextLine S_NextPos=as2_S_NextPos S_Pos=as2_S_Pos S_Reset=as2_S_Reset S_src=as2_S_src)
add_executable(as2_parse main.c)
target_link_libraries(as2_parse PRIVATE as2_frontend)
# Each library owns namespaced globals; serialize calls to the same parser.
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME as2_syntax
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py"
      $<TARGET_FILE:as2_parse>)
endif()

if(BUILD_TESTING)
  add_executable(as2_embed tests/embed.c)
  target_link_libraries(as2_embed PRIVATE as2_frontend)
  add_test(NAME as2_embedding COMMAND as2_embed)
endif()

if(BUILD_TESTING)
  add_test(NAME as2_in_syntax COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/in_check.py $<TARGET_FILE:as2_parse>)
endif()

if(BUILD_TESTING)
  add_test(NAME as2_recovery COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/recovery_check.py $<TARGET_FILE:as2_parse>)
endif()
