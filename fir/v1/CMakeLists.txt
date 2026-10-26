# Independent C99 component. Detached copies need only the root MIT LICENSE.
cmake_minimum_required(VERSION 3.10)
project(fir1 VERSION 0.21.0 LANGUAGES C)
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
add_library(fir1_ir src/ir.c src/pipeline.c src/builder.c optimiser/peephole.c optimiser/effects.c compiler/object.c compiler/dis.c compiler/dis_table.c)
# Preserve object inclusion and private generated headers on CMake 3.10.
target_sources(fir1_ir PRIVATE $<TARGET_OBJECTS:fir1_generated>)
target_include_directories(fir1_ir PRIVATE "${generated}")
target_link_libraries(fir1_ir PRIVATE fir1_io)
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
target_sources(fir1_source_api PRIVATE $<TARGET_OBJECTS:fir1_source_generated>)
target_link_libraries(fir1_source_api PRIVATE fir1_io)
add_executable(fir1_source compiler/source.c)
target_include_directories(fir1_source_generated PRIVATE compiler include)
target_link_libraries(fir1_source PRIVATE fir1_source_api)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(fir1_source PRIVATE -Wall -Wextra -Wpedantic -Werror)
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
  add_test(NAME fir1_acceptance COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_source>)
endif()

# Every generated global is private to its parser namespace.
target_compile_definitions(fir1_generated PRIVATE Custom_Error=fir1_Custom_Error GenError=fir1_GenError Parse=fir1_Parse SemError=fir1_SemError Successful=fir1_Successful SynError=fir1_SynError S_Col=fir1_S_Col S_CurrLine=fir1_S_CurrLine S_CurrentCh=fir1_S_CurrentCh S_Get=fir1_S_Get S_GetName=fir1_S_GetName S_GetString=fir1_S_GetString S_Len=fir1_S_Len S_Line=fir1_S_Line S_LineStart=fir1_S_LineStart S_NextCol=fir1_S_NextCol S_NextLen=fir1_S_NextLen S_NextLine=fir1_S_NextLine S_NextPos=fir1_S_NextPos S_Pos=fir1_S_Pos S_Reset=fir1_S_Reset S_src=fir1_S_src)
target_compile_definitions(fir1_ir PRIVATE Custom_Error=fir1_Custom_Error GenError=fir1_GenError Parse=fir1_Parse SemError=fir1_SemError Successful=fir1_Successful SynError=fir1_SynError S_Col=fir1_S_Col S_CurrLine=fir1_S_CurrLine S_CurrentCh=fir1_S_CurrentCh S_Get=fir1_S_Get S_GetName=fir1_S_GetName S_GetString=fir1_S_GetString S_Len=fir1_S_Len S_Line=fir1_S_Line S_LineStart=fir1_S_LineStart S_NextCol=fir1_S_NextCol S_NextLen=fir1_S_NextLen S_NextLine=fir1_S_NextLine S_NextPos=fir1_S_NextPos S_Pos=fir1_S_Pos S_Reset=fir1_S_Reset S_src=fir1_S_src)
target_compile_definitions(fir1_source_generated PRIVATE Custom_Error=src1_Custom_Error GenError=src1_GenError Parse=src1_Parse SemError=src1_SemError Successful=src1_Successful SynError=src1_SynError S_Col=src1_S_Col S_CurrLine=src1_S_CurrLine S_CurrentCh=src1_S_CurrentCh S_Get=src1_S_Get S_GetName=src1_S_GetName S_GetString=src1_S_GetString S_Len=src1_S_Len S_Line=src1_S_Line S_LineStart=src1_S_LineStart S_NextCol=src1_S_NextCol S_NextLen=src1_S_NextLen S_NextLine=src1_S_NextLine S_NextPos=src1_S_NextPos S_Pos=src1_S_Pos S_Reset=src1_S_Reset S_src=src1_S_src)
target_compile_definitions(fir1_source_api PRIVATE Custom_Error=src1_Custom_Error GenError=src1_GenError Parse=src1_Parse SemError=src1_SemError Successful=src1_Successful SynError=src1_SynError S_Col=src1_S_Col S_CurrLine=src1_S_CurrLine S_CurrentCh=src1_S_CurrentCh S_Get=src1_S_Get S_GetName=src1_S_GetName S_GetString=src1_S_GetString S_Len=src1_S_Len S_Line=src1_S_Line S_LineStart=src1_S_LineStart S_NextCol=src1_S_NextCol S_NextLen=src1_S_NextLen S_NextLine=src1_S_NextLine S_NextPos=src1_S_NextPos S_Pos=src1_S_Pos S_Reset=src1_S_Reset S_src=src1_S_src)
target_compile_options(fir1_source_api PRIVATE $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:-Wall;-Wextra;-Wpedantic;-Werror>)
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

# The structural language frontend is separate from the executable IR verifier/backend.
option(FIR_BUILD_SYNTAX_FRONTEND "Build extended FIR structural syntax API and CLI mode" ON)
if(FIR_BUILD_SYNTAX_FRONTEND)
  add_subdirectory(frontend)
  target_link_libraries(fir1 PRIVATE firsyn_frontend)
  target_compile_definitions(fir1 PRIVATE FIR_HAS_SYNTAX_FRONTEND=1)
endif()

# Object storage names must remain literal AVM1 variables for every accepted module name.
if(BUILD_TESTING)
  add_test(NAME fir1_module_names COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/module_names.py $<TARGET_FILE:fir1_source> $<TARGET_FILE:fir1>)
endif()

# Source lexical scopes must survive real generation, lowering and IR optimization.
if(BUILD_TESTING)
  add_test(NAME fir1_source_scopes COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/scopes.py" $<TARGET_FILE:fir1_source> $<TARGET_FILE:fir1>)
endif()

# First-failure diagnostics are tested through both the public C API and CLI.
if(BUILD_TESTING)
  add_executable(fir1_source_diagnostics tests/source_diagnostics.c)
  target_link_libraries(fir1_source_diagnostics PRIVATE fir1_source_api)
  add_test(NAME fir1_source_diagnostics COMMAND fir1_source_diagnostics)
  add_test(NAME fir1_source_json COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_json.py" $<TARGET_FILE:fir1_source>)
endif()

# A standalone commented C99 client demonstrates copied diagnostic ownership.
add_executable(fir1_diagnostic_example examples/diagnostics/main.c)
target_link_libraries(fir1_diagnostic_example PRIVATE fir1_source_api)
if(BUILD_TESTING)
  add_test(NAME fir1_diagnostic_example COMMAND fir1_diagnostic_example)
endif()
if(BUILD_TESTING)
  add_test(NAME fir1_source_debug COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_debug.py" $<TARGET_FILE:fir1_source> $<TARGET_FILE:fir1>)
endif()
if(BUILD_TESTING)
  add_executable(fir1_source_debug_api tests/source_debug_api.c)
  target_link_libraries(fir1_source_debug_api PRIVATE fir1_source_api)
  add_test(NAME fir1_source_debug_api COMMAND fir1_source_debug_api)
endif()

# The direct backend remains independently testable without DisVM or an Inferno SDK.
if(BUILD_TESTING)
  add_test(NAME fir1_dis_backend COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/dis_backend.py" $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_source>)
endif()

if(BUILD_TESTING)
  add_executable(fir1_dis_api tests/dis_api.c)
  target_link_libraries(fir1_dis_api PRIVATE fir1_ir)
  add_test(NAME fir1_dis_api COMMAND fir1_dis_api)
endif()

# Runnable embedding client links our libraries directly; it never shells out to limbo-native.
add_executable(fir1_dis_example examples/dis/main.c)
target_link_libraries(fir1_dis_example PRIVATE fir1_ir fir1_source_api)

# Terminal source decisions must retain explicit CFG and authored comparison origins.
if(BUILD_TESTING)
  add_test(NAME fir1_source_decisions COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/decisions.py" $<TARGET_FILE:fir1_source> $<TARGET_FILE:fir1>)
endif()

if(BUILD_TESTING)
  add_test(NAME fir1_limbo_reals COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/limbo_reals.py" $<TARGET_FILE:fir1_source> $<TARGET_FILE:fir1>)
endif()

if(BUILD_TESTING)
  add_executable(fir1_real_api tests/real_api.c)
  target_link_libraries(fir1_real_api PRIVATE fir1_ir fir1_source_api)
  add_test(NAME fir1_real_api COMMAND fir1_real_api)
endif()

if(BUILD_TESTING)
  add_test(NAME fir1_access_sites COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/access_sites.py" $<TARGET_FILE:fir1_source>)
endif()

# Optional reference execution takes an explicit independently supplied C99 table core.
set(FIR_TABLE_CORE_ROOT "" CACHE PATH "Directory containing the matching table.c/table.h")
if(FIR_TABLE_CORE_ROOT)
  if(NOT EXISTS "${FIR_TABLE_CORE_ROOT}/table.c" OR NOT EXISTS "${FIR_TABLE_CORE_ROOT}/table.h")
    message(FATAL_ERROR "FIR_TABLE_CORE_ROOT must contain table.c and table.h")
  endif()
  add_library(fir1_tables compiler/table_run.c "${FIR_TABLE_CORE_ROOT}/table.c")
  target_include_directories(fir1_tables PUBLIC include PRIVATE "${FIR_TABLE_CORE_ROOT}")
  target_link_libraries(fir1_tables PUBLIC fir1_ir)
  if(BUILD_TESTING)
    add_test(NAME fir1_table_cfg COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/table_cfg.py" $<TARGET_FILE:fir1_source>
      $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_table_run>)
    add_test(NAME fir1_typed_tables COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/tables.py" $<TARGET_FILE:fir1_source>
      $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_table_run>)
  endif()
  add_executable(fir1_table_run compiler/table_main.c)
  target_link_libraries(fir1_table_run PRIVATE fir1_tables)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fir1_tables PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(fir1_table_run PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endif()

# Big terminal decisions use the authoritative source grammar and direct scalar Dis backend.
if(BUILD_TESTING)
  add_test(NAME fir1_big_decisions COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/big_decisions.py" $<TARGET_FILE:fir1_source> $<TARGET_FILE:fir1>)
endif()

# Typed long shifts retain int count semantics even in the independent compiler build.
if(BUILD_TESTING)
  if(FIR_TABLE_CORE_ROOT)
    add_test(NAME fir1_big_shifts COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/big_shifts.py" $<TARGET_FILE:fir1_source>
      $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_table_run>)
  else()
    add_test(NAME fir1_big_shifts COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/big_shifts.py" $<TARGET_FILE:fir1_source> $<TARGET_FILE:fir1> -)
  endif()
endif()

# New table operations are exercised through the authoritative Coco/R source and FIR APIs.
if(BUILD_TESTING)
  if(FIR_TABLE_CORE_ROOT)
    add_test(NAME fir1_table_mutation COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/table_mutation.py" $<TARGET_FILE:fir1_source>
      $<TARGET_FILE:fir1> $<TARGET_FILE:fir1_table_run>)
  else()
    add_test(NAME fir1_table_mutation COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/table_mutation.py" $<TARGET_FILE:fir1_source>
      $<TARGET_FILE:fir1> -)
  endif()
endif()
