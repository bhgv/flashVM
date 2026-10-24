# Standalone C99 syntax frontend; Coco/R is an explicit external build tool.
cmake_minimum_required(VERSION 3.16)
project(limbo_frontend VERSION 0.6.5 LANGUAGES C)
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
  COMMAND ${CMAKE_COMMAND} "-DCOCOR_EXECUTABLE=${COCOR_EXECUTABLE}"
    "-DCOCOR_ROOT=${COCOR_ROOT}" -DGRAMMAR=limbo
    -P "${CMAKE_CURRENT_SOURCE_DIR}/run_cocor.cmake"
  WORKING_DIRECTORY "${generated}"
  DEPENDS
    run_cocor.cmake
    limbo.atg
    "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm"
    "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm"
    "${COCOR_ROOT}/frames/parser_h.frm"
  VERBATIM)
set_source_files_properties("${generated}/limbop.c" "${generated}/limbos.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(limbo_frontend api.c tree.c parser.c scanner.c "${generated}/limbop.c" "${generated}/limbos.c")
target_include_directories(limbo_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(limbo_frontend PRIVATE "${generated}")
target_compile_definitions(limbo_frontend PRIVATE CUSTOM_BUFF Custom_Error=limbo_Custom_Error GenError=limbo_GenError Parse=limbo_Parse SemError=limbo_SemError Successful=limbo_Successful SynError=limbo_SynError S_Col=limbo_S_Col S_CurrLine=limbo_S_CurrLine S_CurrentCh=limbo_S_CurrentCh S_Get=limbo_S_Get S_GetName=limbo_S_GetName S_GetString=limbo_S_GetString S_Len=limbo_S_Len S_Line=limbo_S_Line S_LineStart=limbo_S_LineStart S_NextCol=limbo_S_NextCol S_NextLen=limbo_S_NextLen S_NextLine=limbo_S_NextLine S_NextPos=limbo_S_NextPos S_Pos=limbo_S_Pos S_Reset=limbo_S_Reset S_src=limbo_S_src)
add_executable(limbo_parse main.c)
target_link_libraries(limbo_parse PRIVATE limbo_frontend)
# Each library owns namespaced globals; serialize calls to the same parser.
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME limbo_syntax
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py"
      $<TARGET_FILE:limbo_parse>)
endif()

if(BUILD_TESTING)
  add_executable(limbo_embed tests/embed.c)
  target_link_libraries(limbo_embed PRIVATE limbo_frontend)
  add_test(NAME limbo_embedding COMMAND limbo_embed)
endif()

if(BUILD_TESTING)
  add_test(NAME limbo_recovery COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/recovery_check.py $<TARGET_FILE:limbo_parse>)
endif()

# Exercise the host generator failure boundary, not the target CLI.
if(BUILD_TESTING)
  add_test(NAME limbo_generator_failure COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/generator_check.py ${CMAKE_COMMAND}
    ${CMAKE_CURRENT_SOURCE_DIR}/run_cocor.cmake ${COCOR_EXECUTABLE} ${COCOR_ROOT})
endif()

# Handwritten owned storage is checked independently of generated legacy runtime warnings.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(tree.c PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
endif()

# The tree inspector is separate from the compatibility syntax-only CLI.
add_executable(limbo_tree tree_main.c)
target_link_libraries(limbo_tree PRIVATE limbo_frontend)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(limbo_tree PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING)
  add_test(NAME limbo_tree_structure COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tree_check.py $<TARGET_FILE:limbo_tree>)
endif()

if(BUILD_TESTING)
  add_executable(limbo_tree_embed tests/tree_embed.c)
  target_link_libraries(limbo_tree_embed PRIVATE limbo_frontend)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(limbo_tree_embed PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME limbo_tree_ownership COMMAND limbo_tree_embed)
endif()

if(BUILD_TESTING)
  add_executable(limbo_tree_locate tests/tree_locate.c)
  target_link_libraries(limbo_tree_locate PRIVATE limbo_frontend)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(limbo_tree_locate PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME limbo_tree_source_context COMMAND limbo_tree_locate)
endif()

if(BUILD_TESTING)
  # Override allocation only in the owned store; generated scanner allocation is separate.
  add_library(limbo_tree_fault_store OBJECT tree.c)
  target_include_directories(limbo_tree_fault_store PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(limbo_tree_fault_store PRIVATE
    malloc=limbo_test_malloc calloc=limbo_test_calloc realloc=limbo_test_realloc)
  add_executable(limbo_tree_alloc tests/tree_alloc.c $<TARGET_OBJECTS:limbo_tree_fault_store>)
  target_link_libraries(limbo_tree_alloc PRIVATE limbo_frontend)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(limbo_tree_alloc PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME limbo_tree_allocation_recovery COMMAND limbo_tree_alloc)
endif()

# The declaration catalog consumes the existing Coco/R tree, not a second parser.
add_library(limbo_interfaces interface.c)
target_link_libraries(limbo_interfaces PUBLIC limbo_frontend)
target_include_directories(limbo_interfaces PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(limbo_interfaces PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_executable(limbo_interface interface_main.c)
target_link_libraries(limbo_interface PRIVATE limbo_interfaces)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(limbo_interface PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
if(BUILD_TESTING)
  add_executable(limbo_interface_embed tests/interface_embed.c)
  target_link_libraries(limbo_interface_embed PRIVATE limbo_interfaces)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(limbo_interface_embed PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME limbo_interface_ownership COMMAND limbo_interface_embed)
  add_test(NAME limbo_interface_catalog COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/interface_check.py $<TARGET_FILE:limbo_interface>)
endif()
if(BUILD_TESTING)
  add_executable(limbo_interface_lookup tests/interface_lookup.c)
  target_link_libraries(limbo_interface_lookup PRIVATE limbo_interfaces)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(limbo_interface_lookup PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME limbo_interface_export_lookup COMMAND limbo_interface_lookup)
endif()
if(BUILD_TESTING)
  add_library(limbo_interface_fault_store OBJECT interface.c)
  target_include_directories(limbo_interface_fault_store PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(limbo_interface_fault_store PRIVATE malloc=limbo_interface_test_malloc
    calloc=limbo_interface_test_calloc realloc=limbo_interface_test_realloc free=limbo_interface_test_free)
  add_executable(limbo_interface_alloc tests/interface_alloc.c $<TARGET_OBJECTS:limbo_interface_fault_store>)
  target_link_libraries(limbo_interface_alloc PRIVATE limbo_interfaces)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(limbo_interface_alloc PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(limbo_interface_fault_store PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME limbo_interface_allocation_recovery COMMAND limbo_interface_alloc)
endif()

# Optional external checkout corpus; never download or vendor it during a build.
set(LIMBO_INTERFACE_UPSTREAM "" CACHE PATH "Explicit Inferno checkout for optional .m/native-name tests")
if(BUILD_TESTING AND LIMBO_INTERFACE_UPSTREAM)
  add_test(NAME limbo_interface_upstream COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/interface_upstream.py --upstream ${LIMBO_INTERFACE_UPSTREAM}
    --tool $<TARGET_FILE:limbo_interface>)
endif()
