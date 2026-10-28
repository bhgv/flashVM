# Independently buildable Coco/R display profile; no deployed parser/compiler dependency.
cmake_minimum_required(VERSION 3.10)
project(as2_view LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(COCOR_ROOT "" CACHE PATH "Explicit Coco/R SDK root with frames/")
set(COCOR_EXECUTABLE "" CACHE FILEPATH "Host Coco/R executable")
if(NOT EXISTS "${COCOR_EXECUTABLE}" OR NOT EXISTS "${COCOR_ROOT}/frames/scan_c.frm")
  message(FATAL_ERROR "Supply COCOR_ROOT and COCOR_EXECUTABLE")
endif()
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${generated}")
add_custom_command(OUTPUT "${generated}/viewp.c" "${generated}/views.c"
  "${generated}/viewp.h" "${generated}/views.h" "${generated}/viewc.h" "${generated}/viewe.h"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/view.atg" "${generated}/view.atg"
  COMMAND ${CMAKE_COMMAND} "-DCOCOR_EXECUTABLE=${COCOR_EXECUTABLE}" "-DCOCOR_ROOT=${COCOR_ROOT}"
    -DGRAMMAR=view -P "${CMAKE_CURRENT_SOURCE_DIR}/../frontend/run_cocor.cmake"
  WORKING_DIRECTORY "${generated}"
  DEPENDS view.atg "${CMAKE_CURRENT_SOURCE_DIR}/../frontend/run_cocor.cmake" "${COCOR_EXECUTABLE}"
    "${COCOR_ROOT}/frames/scan_c.frm" "${COCOR_ROOT}/frames/scan_h.frm"
    "${COCOR_ROOT}/frames/parser_c.frm" "${COCOR_ROOT}/frames/parser_h.frm" VERBATIM)
if(NOT TARGET as2_frontend)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../frontend" syntax)
endif()
set_source_files_properties("${generated}/viewp.c" "${generated}/views.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(as2_view emit.c parser.c scanner.c "${generated}/viewp.c" "${generated}/views.c")
target_link_libraries(as2_view PUBLIC as2_frontend)
add_executable(as2_viewc main.c)
target_link_libraries(as2_viewc PRIVATE as2_view)
target_compile_definitions(as2_view PRIVATE CUSTOM_BUFF)
target_include_directories(as2_view PRIVATE "${generated}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Keep the generated C API linkable beside the full AS2 and other language parsers.
target_compile_definitions(as2_view PRIVATE
  Custom_Error=view_Custom_Error GenError=view_GenError Parse=view_Parse
  SemError=view_SemError Successful=view_Successful SynError=view_SynError
  S_Col=view_S_Col S_CurrLine=view_S_CurrLine S_CurrentCh=view_S_CurrentCh
  S_Get=view_S_Get S_GetName=view_S_GetName S_GetString=view_S_GetString
  S_Len=view_S_Len S_Line=view_S_Line S_LineStart=view_S_LineStart
  S_NextCol=view_S_NextCol S_NextLen=view_S_NextLen S_NextLine=view_S_NextLine
  S_NextPos=view_S_NextPos S_Pos=view_S_Pos S_Reset=view_S_Reset S_src=view_S_src)
include(CTest)
if(BUILD_TESTING)
  add_executable(as2_view_embed tests.c)
  target_link_libraries(as2_view_embed PRIVATE as2_view)
  add_test(NAME as2_view_embedding COMMAND as2_view_embed)
endif()
