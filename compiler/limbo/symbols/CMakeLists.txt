# Independent C99 symbol source-map component; generator is an explicit host tool.
cmake_minimum_required(VERSION 3.10)
project(limbo_sblmap VERSION 0.1.24 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(COCOR_ROOT "" CACHE PATH "External bhgv/coco-r-C root")
set(COCOR_EXECUTABLE "" CACHE FILEPATH "External host C Coco/R executable")
if(NOT EXISTS "${COCOR_EXECUTABLE}" OR NOT EXISTS "${COCOR_ROOT}/frames/scan_c.frm")
  message(FATAL_ERROR "Provide COCOR_ROOT and COCOR_EXECUTABLE")
endif()
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${generated}")
add_custom_command(OUTPUT "${generated}/sblmapp.c" "${generated}/sblmaps.c" "${generated}/sblmapp.h" "${generated}/sblmapc.h" "${generated}/sblmaps.h" "${generated}/sblmape.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/sblmap.atg" "${generated}/sblmap.atg"
  COMMAND ${CMAKE_COMMAND} -DCOCOR_ROOT=${COCOR_ROOT} -DCOCOR_EXECUTABLE=${COCOR_EXECUTABLE} -DGRAMMAR=sblmap -P "${CMAKE_CURRENT_SOURCE_DIR}/run_cocor.cmake"
  DEPENDS sblmap.atg run_cocor.cmake "${COCOR_EXECUTABLE}" "${COCOR_ROOT}/frames/parser_c.frm" "${COCOR_ROOT}/frames/parser_h.frm" "${COCOR_ROOT}/frames/scan_c.frm" "${COCOR_ROOT}/frames/scan_h.frm"
  WORKING_DIRECTORY "${generated}" VERBATIM)
set_source_files_properties("${generated}/sblmapp.c" "${generated}/sblmaps.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(sblmap_generated OBJECT src/parser.c src/scanner.c "${generated}/sblmapp.c" "${generated}/sblmaps.c")
target_include_directories(sblmap_generated PRIVATE include src "${generated}")
target_compile_definitions(sblmap_generated PRIVATE CUSTOM_BUFF)
add_library(limbo_sblmap src/map.c src/queries.c)
target_include_directories(limbo_sblmap PUBLIC include PRIVATE src "${generated}")
# Preserve object inclusion and private generated headers on CMake 3.10.
target_sources(limbo_sblmap PRIVATE $<TARGET_OBJECTS:sblmap_generated>)
target_include_directories(limbo_sblmap PRIVATE "${generated}")
target_compile_definitions(limbo_sblmap PRIVATE CUSTOM_BUFF)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(limbo_sblmap PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
# Avoid exporting the generator's unqualified process-global symbol names.
foreach(target sblmap_generated limbo_sblmap)
  target_compile_definitions(${target} PRIVATE
    Custom_Error=sbl_Custom_Error
    GenError=sbl_GenError
    Parse=sbl_Parse
    SemError=sbl_SemError
    Successful=sbl_Successful
    SynError=sbl_SynError
    S_Col=sbl_S_Col
    S_CurrLine=sbl_S_CurrLine
    S_CurrentCh=sbl_S_CurrentCh
    S_Get=sbl_S_Get
    S_GetName=sbl_S_GetName
    S_GetString=sbl_S_GetString
    S_Len=sbl_S_Len
    S_Line=sbl_S_Line
    S_LineStart=sbl_S_LineStart
    S_NextCol=sbl_S_NextCol
    S_NextLen=sbl_S_NextLen
    S_NextLine=sbl_S_NextLine
    S_NextPos=sbl_S_NextPos
    S_Pos=sbl_S_Pos
    S_Reset=sbl_S_Reset
    S_src=sbl_S_src
  )
endforeach()
add_executable(sblmap src/main.c)
target_include_directories(sblmap PRIVATE src)
target_link_libraries(sblmap PRIVATE limbo_sblmap)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(sblmap PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_executable(sblquery src/query_main.c)
target_include_directories(sblquery PRIVATE src)
target_link_libraries(sblquery PRIVATE limbo_sblmap)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(sblquery PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
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
  add_executable(sblmap_embed tests/embed.c)
  target_link_libraries(sblmap_embed PRIVATE limbo_sblmap)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(sblmap_embed PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_executable(sblmap_queries tests/queries.c)
  target_link_libraries(sblmap_queries PRIVATE limbo_sblmap)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(sblmap_queries PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME sblmap_query_contracts COMMAND sblmap_queries)
  add_test(NAME sblmap_embedding COMMAND sblmap_embed)
  add_test(NAME sblmap_source_prefix COMMAND ${Python3_EXECUTABLE} -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/tests" -v)
  set_tests_properties(sblmap_source_prefix PROPERTIES ENVIRONMENT "SBLMAP_TOOL=$<TARGET_FILE:sblmap>;SBLQUERY_TOOL=$<TARGET_FILE:sblquery>")
endif()
