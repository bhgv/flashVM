# Selected upstream generator sources, built out of tree without SDK downloads.
cmake_minimum_required(VERSION 3.16)
project(coco_r_limbo LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_executable(cocor
  upstream/sources/limbo.c
  upstream/sources/cr.c upstream/sources/cra.c upstream/sources/crp.c
  upstream/sources/crs.c upstream/sources/crt.c upstream/sources/crx.c
  upstream/sources/crf.c upstream/sources/collect.c upstream/sources/set.c)
# Preserve upstream warning policy; handwritten adapters are reviewed separately.
target_include_directories(cocor PRIVATE upstream/sources)

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME cocor_generation COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.py" $<TARGET_FILE:cocor>
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/frames")
  set_tests_properties(cocor_generation PROPERTIES TIMEOUT 60
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

set(LIMBO_EXECUTABLE "" CACHE FILEPATH "Optional native Limbo bootstrap for cocor.dis")
set(LIMBO_MODULE_DIR "" CACHE PATH "Explicit module headers for the Limbo generator")
if(LIMBO_EXECUTABLE)
  if(NOT EXISTS "${LIMBO_EXECUTABLE}" OR NOT EXISTS "${LIMBO_MODULE_DIR}/sys.m")
    message(FATAL_ERROR "Provide native LIMBO_EXECUTABLE and LIMBO_MODULE_DIR")
  endif()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cocor.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${LIMBO_MODULE_DIR}" -o "${CMAKE_CURRENT_BINARY_DIR}/cocor.dis"
      "${CMAKE_CURRENT_SOURCE_DIR}/upstream/limbo/cocor.b"
    DEPENDS upstream/limbo/cocor.b "${LIMBO_EXECUTABLE}" "${LIMBO_MODULE_DIR}/sys.m"
      "${LIMBO_MODULE_DIR}/draw.m" VERBATIM)
  add_custom_target(cocor_dis ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/cocor.dis")
endif()
