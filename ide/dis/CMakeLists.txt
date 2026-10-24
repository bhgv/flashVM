# Independent build of the portable Limbo module; native compiler is an explicit host dependency.
cmake_minimum_required(VERSION 3.16)
project(flashvm_ide_dis VERSION 0.2.0 LANGUAGES NONE)
set(LIMBO_EXECUTABLE "" CACHE FILEPATH "Explicit trusted native Limbo compiler")
if(NOT EXISTS "${LIMBO_EXECUTABLE}")
  message(FATAL_ERROR "Provide LIMBO_EXECUTABLE; this build never downloads a compiler")
endif()
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/idecore.dis" "${CMAKE_CURRENT_BINARY_DIR}/idecore.sbl"
  COMMAND "${LIMBO_EXECUTABLE}" -g -I "${CMAKE_CURRENT_SOURCE_DIR}"
    -o "${CMAKE_CURRENT_BINARY_DIR}/idecore.dis" "${CMAKE_CURRENT_SOURCE_DIR}/idecore.b"
  DEPENDS idecore.b idecore.m "${LIMBO_EXECUTABLE}" VERBATIM)
add_custom_target(ide_dis ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/idecore.dis"
  "${CMAKE_CURRENT_BINARY_DIR}/idecore.sbl")
