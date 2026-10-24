# Independent build of the portable Limbo module; native compiler is an explicit host dependency.
cmake_minimum_required(VERSION 3.16)
project(flashvm_ide_dis VERSION 0.2.1 LANGUAGES NONE)
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

include(CTest)
set(EMU_EXECUTABLE "" CACHE FILEPATH "Optional explicit reference Dis emulator for channel regression")
set(LIMBO_MODULE_DIR "" CACHE PATH "External module headers for the test bootstrap's Sys interface")
if(BUILD_TESTING AND EMU_EXECUTABLE)
  if(NOT EXISTS "${EMU_EXECUTABLE}" OR NOT EXISTS "${LIMBO_MODULE_DIR}/sys.m")
    message(FATAL_ERROR "Provide reference EMU_EXECUTABLE and LIMBO_MODULE_DIR")
  endif()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/idecore_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/idecore_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/idecore_test.b"
    DEPENDS tests/idecore_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_test_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/idecore_test.dis")
  foreach(mode 0 1)
    add_test(NAME ide_dis_reference_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module idecore_test.dis)
  endforeach()
  set_tests_properties(ide_dis_reference_mode_0 ide_dis_reference_mode_1 PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
endif()
