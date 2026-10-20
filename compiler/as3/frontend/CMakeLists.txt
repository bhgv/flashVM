# Standalone C99 syntax frontend; Coco/R is an explicit external build tool.
cmake_minimum_required(VERSION 3.16)
project(as3_frontend VERSION 0.2.1 LANGUAGES C)
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
    "${generated}/as3p.c"
    "${generated}/as3s.c"
    "${generated}/as3p.h"
    "${generated}/as3s.h"
    "${generated}/as3c.h"
    "${generated}/as3e.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/as3.atg"
    "${generated}/as3.atg"
  COMMAND ${CMAKE_COMMAND} -E env "CRFRAMES=${COCOR_ROOT}/frames" "${COCOR_EXECUTABLE}" as3.atg
  WORKING_DIRECTORY "${generated}"
  DEPENDS
    as3.atg
    "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm"
    "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm"
    "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
set_source_files_properties("${generated}/as3p.c" "${generated}/as3s.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(as3_frontend api.c parser.c scanner.c "${generated}/as3p.c" "${generated}/as3s.c")
target_include_directories(as3_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(as3_frontend PRIVATE "${generated}")
target_compile_definitions(as3_frontend PRIVATE CUSTOM_BUFF Custom_Error=as3_Custom_Error GenError=as3_GenError Parse=as3_Parse SemError=as3_SemError Successful=as3_Successful SynError=as3_SynError S_Col=as3_S_Col S_CurrLine=as3_S_CurrLine S_CurrentCh=as3_S_CurrentCh S_Get=as3_S_Get S_GetName=as3_S_GetName S_GetString=as3_S_GetString S_Len=as3_S_Len S_Line=as3_S_Line S_LineStart=as3_S_LineStart S_NextCol=as3_S_NextCol S_NextLen=as3_S_NextLen S_NextLine=as3_S_NextLine S_NextPos=as3_S_NextPos S_Pos=as3_S_Pos S_Reset=as3_S_Reset S_src=as3_S_src)
add_executable(as3_parse main.c)
target_link_libraries(as3_parse PRIVATE as3_frontend)
# Each library owns namespaced globals; serialize calls to the same parser.
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME as3_syntax
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py"
      $<TARGET_FILE:as3_parse>)
endif()

if(BUILD_TESTING)
  add_executable(as3_embed tests/embed.c)
  target_link_libraries(as3_embed PRIVATE as3_frontend)
  add_test(NAME as3_embedding COMMAND as3_embed)
endif()

if(BUILD_TESTING)
  add_test(NAME as3_in_syntax COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/in_check.py $<TARGET_FILE:as3_parse>)
endif()
