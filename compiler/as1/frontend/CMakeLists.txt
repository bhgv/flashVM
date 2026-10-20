# Standalone C99 syntax frontend; Coco/R is an explicit external build tool.
cmake_minimum_required(VERSION 3.16)
project(as1_frontend VERSION 0.2.5 LANGUAGES C)
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
    "${generated}/as1p.c"
    "${generated}/as1s.c"
    "${generated}/as1p.h"
    "${generated}/as1s.h"
    "${generated}/as1c.h"
    "${generated}/as1e.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/as1.atg"
    "${generated}/as1.atg"
  COMMAND ${CMAKE_COMMAND} -E env "CRFRAMES=${COCOR_ROOT}/frames" "${COCOR_EXECUTABLE}" as1.atg
  WORKING_DIRECTORY "${generated}"
  DEPENDS
    as1.atg
    "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm"
    "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm"
    "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
set_source_files_properties("${generated}/as1p.c" "${generated}/as1s.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(as1_frontend api.c parser.c scanner.c "${generated}/as1p.c" "${generated}/as1s.c")
target_include_directories(as1_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(as1_frontend PRIVATE "${generated}")
target_compile_definitions(as1_frontend PRIVATE CUSTOM_BUFF Custom_Error=as1_Custom_Error GenError=as1_GenError Parse=as1_Parse SemError=as1_SemError Successful=as1_Successful SynError=as1_SynError S_Col=as1_S_Col S_CurrLine=as1_S_CurrLine S_CurrentCh=as1_S_CurrentCh S_Get=as1_S_Get S_GetName=as1_S_GetName S_GetString=as1_S_GetString S_Len=as1_S_Len S_Line=as1_S_Line S_LineStart=as1_S_LineStart S_NextCol=as1_S_NextCol S_NextLen=as1_S_NextLen S_NextLine=as1_S_NextLine S_NextPos=as1_S_NextPos S_Pos=as1_S_Pos S_Reset=as1_S_Reset S_src=as1_S_src)
add_executable(as1_parse main.c)
target_link_libraries(as1_parse PRIVATE as1_frontend)
# Each library owns namespaced globals; serialize calls to the same parser.
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME as1_syntax
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py"
      $<TARGET_FILE:as1_parse>)
endif()

if(BUILD_TESTING)
  add_executable(as1_embed tests/embed.c)
  target_link_libraries(as1_embed PRIVATE as1_frontend)
  add_test(NAME as1_embedding COMMAND as1_embed)
endif()

if(BUILD_TESTING)
  add_test(NAME as1_in_syntax COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/in_check.py $<TARGET_FILE:as1_parse>)
endif()

if(BUILD_TESTING)
  add_test(NAME as1_recovery COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/recovery_check.py $<TARGET_FILE:as1_parse>)
endif()
