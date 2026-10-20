# Independent C99 component. Detached copies need only the root MIT LICENSE.
cmake_minimum_required(VERSION 3.16)
project(fir1 VERSION 0.4.5 LANGUAGES C)
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
set_source_files_properties("${generated}/fir1p.c" "${generated}/src1p.c" "${generated}/fir1s.c" "${generated}/src1s.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(fir1_generated OBJECT src/parser.c src/scanner.c "${generated}/fir1p.c" "${generated}/fir1s.c")
target_compile_definitions(fir1_generated PRIVATE CUSTOM_BUFF)
target_include_directories(fir1_generated PRIVATE include src)
target_include_directories(fir1_generated PUBLIC "${generated}")
add_library(fir1_io src/input.c)
add_library(fir1_ir src/ir.c src/pipeline.c src/builder.c optimiser/peephole.c optimiser/effects.c compiler/object.c)
target_link_libraries(fir1_ir PRIVATE fir1_generated fir1_io)
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
add_library(fir1_source_generated OBJECT compiler/source_parser.c compiler/source_scanner.c "${generated}/src1p.c" "${generated}/src1s.c")
target_compile_definitions(fir1_source_generated PRIVATE CUSTOM_BUFF)
target_include_directories(fir1_source_generated PUBLIC "${generated}")
add_library(fir1_source_api compiler/source_api.c compiler/lower.c)
target_include_directories(fir1_source_api PUBLIC include)
target_include_directories(fir1_source_api PRIVATE "${generated}")
target_link_libraries(fir1_source_api PRIVATE fir1_source_generated fir1_io)
add_executable(fir1_source compiler/source.c)
target_include_directories(fir1_source_generated PRIVATE compiler)
target_link_libraries(fir1_source PRIVATE fir1_source_api)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(fir1_source PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME fir1_acceptance COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_source>)
endif()

# Every generated global is private to its parser namespace.
target_compile_definitions(fir1_generated PRIVATE Custom_Error=fir1_Custom_Error GenError=fir1_GenError Parse=fir1_Parse SemError=fir1_SemError Successful=fir1_Successful SynError=fir1_SynError S_Col=fir1_S_Col S_CurrLine=fir1_S_CurrLine S_CurrentCh=fir1_S_CurrentCh S_Get=fir1_S_Get S_GetName=fir1_S_GetName S_GetString=fir1_S_GetString S_Len=fir1_S_Len S_Line=fir1_S_Line S_LineStart=fir1_S_LineStart S_NextCol=fir1_S_NextCol S_NextLen=fir1_S_NextLen S_NextLine=fir1_S_NextLine S_NextPos=fir1_S_NextPos S_Pos=fir1_S_Pos S_Reset=fir1_S_Reset S_src=fir1_S_src)
target_compile_definitions(fir1_ir PRIVATE Custom_Error=fir1_Custom_Error GenError=fir1_GenError Parse=fir1_Parse SemError=fir1_SemError Successful=fir1_Successful SynError=fir1_SynError S_Col=fir1_S_Col S_CurrLine=fir1_S_CurrLine S_CurrentCh=fir1_S_CurrentCh S_Get=fir1_S_Get S_GetName=fir1_S_GetName S_GetString=fir1_S_GetString S_Len=fir1_S_Len S_Line=fir1_S_Line S_LineStart=fir1_S_LineStart S_NextCol=fir1_S_NextCol S_NextLen=fir1_S_NextLen S_NextLine=fir1_S_NextLine S_NextPos=fir1_S_NextPos S_Pos=fir1_S_Pos S_Reset=fir1_S_Reset S_src=fir1_S_src)
target_compile_definitions(fir1_source_generated PRIVATE Custom_Error=src1_Custom_Error GenError=src1_GenError Parse=src1_Parse SemError=src1_SemError Successful=src1_Successful SynError=src1_SynError S_Col=src1_S_Col S_CurrLine=src1_S_CurrLine S_CurrentCh=src1_S_CurrentCh S_Get=src1_S_Get S_GetName=src1_S_GetName S_GetString=src1_S_GetString S_Len=src1_S_Len S_Line=src1_S_Line S_LineStart=src1_S_LineStart S_NextCol=src1_S_NextCol S_NextLen=src1_S_NextLen S_NextLine=src1_S_NextLine S_NextPos=src1_S_NextPos S_Pos=src1_S_Pos S_Reset=src1_S_Reset S_src=src1_S_src)
target_compile_definitions(fir1_source_api PRIVATE Custom_Error=src1_Custom_Error GenError=src1_GenError Parse=src1_Parse SemError=src1_SemError Successful=src1_Successful SynError=src1_SynError S_Col=src1_S_Col S_CurrLine=src1_S_CurrLine S_CurrentCh=src1_S_CurrentCh S_Get=src1_S_Get S_GetName=src1_S_GetName S_GetString=src1_S_GetString S_Len=src1_S_Len S_Line=src1_S_Line S_LineStart=src1_S_LineStart S_NextCol=src1_S_NextCol S_NextLen=src1_S_NextLen S_NextLine=src1_S_NextLine S_NextPos=src1_S_NextPos S_Pos=src1_S_Pos S_Reset=src1_S_Reset S_src=src1_S_src)
target_compile_options(fir1_source_api PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>)
add_executable(fir1_opt optimiser/main.c)
target_link_libraries(fir1_opt PRIVATE fir1_ir)
if(BUILD_TESTING)
  add_executable(fir1_embed tests/embed.c)
  target_link_libraries(fir1_embed PRIVATE fir1_ir fir1_source_api)
  add_test(NAME fir1_embedding COMMAND fir1_embed)
endif()
if(BUILD_TESTING)
  add_executable(fir1_verify_api tests/verify_api.c)
  target_link_libraries(fir1_verify_api PRIVATE fir1_ir)
  add_test(NAME fir1_verify_api COMMAND fir1_verify_api)
endif()

# Generated parser configuration is never part of the embedding interface.
target_compile_definitions(fir1_ir PRIVATE CUSTOM_BUFF)
target_compile_definitions(fir1_source_api PRIVATE CUSTOM_BUFF)
