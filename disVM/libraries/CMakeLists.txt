# Selected Limbo libraries build independently with an explicit compiler/source closure.
cmake_minimum_required(VERSION 3.10)
project(disvm_libraries VERSION 0.1.0 LANGUAGES NONE)
find_package(PythonInterp 3 REQUIRED)
set(LIMBO_EXECUTABLE "" CACHE FILEPATH "Trusted source-built matching Limbo compiler")
set(DVM_LIBRARY_SOURCE_ROOT "" CACHE PATH "Selected Inferno closure containing appl/lib and module")
set(DVM_LIBRARY_CELL_BYTES "4" CACHE STRING "Matching compiler cell width, 4 or experimental 8")
if(NOT EXISTS "${LIMBO_EXECUTABLE}" OR NOT EXISTS "${DVM_LIBRARY_SOURCE_ROOT}/module/sys.m")
  message(FATAL_ERROR "Provide LIMBO_EXECUTABLE and DVM_LIBRARY_SOURCE_ROOT explicitly")
endif()
if(NOT DVM_LIBRARY_CELL_BYTES MATCHES "^(4|8)$")
  message(FATAL_ERROR "DVM_LIBRARY_CELL_BYTES must be 4 or 8")
endif()
set(prepared "${CMAKE_CURRENT_BINARY_DIR}/source")
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare.py"
  --source "${DVM_LIBRARY_SOURCE_ROOT}" --output "${prepared}" --cells "${DVM_LIBRARY_CELL_BYTES}"
  RESULT_VARIABLE status)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "Selected library preparation failed")
endif()
file(GLOB sources "${DVM_LIBRARY_SOURCE_ROOT}/appl/lib/*.b")
file(GLOB headers "${DVM_LIBRARY_SOURCE_ROOT}/module/*.m")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${sources}
  "${CMAKE_CURRENT_SOURCE_DIR}/prepare.py")
set(products)
foreach(name bufio string arg readdir debug dis styx styxservers regex)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.dis" "${CMAKE_CURRENT_BINARY_DIR}/${name}.sbl"
    COMMAND "${LIMBO_EXECUTABLE}" -g -I "${DVM_LIBRARY_SOURCE_ROOT}/module"
      -o "${CMAKE_CURRENT_BINARY_DIR}/${name}.dis" "${prepared}/${name}.b"
    DEPENDS "${prepared}/${name}.b" ${headers} "${LIMBO_EXECUTABLE}" VERBATIM)
  list(APPEND products "${CMAKE_CURRENT_BINARY_DIR}/${name}.dis" "${CMAKE_CURRENT_BINARY_DIR}/${name}.sbl")
endforeach()
add_custom_target(disvm_libraries ALL DEPENDS ${products})
