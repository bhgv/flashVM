# Selected upstream generator sources, built out of tree without SDK downloads.
cmake_minimum_required(VERSION 3.10)
project(coco_r_C LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_executable(cocor
  upstream/sources/cr.c upstream/sources/cra.c upstream/sources/crp.c
  upstream/sources/crs.c upstream/sources/crt.c upstream/sources/crx.c
  upstream/sources/crf.c upstream/sources/collect.c upstream/sources/set.c)
# Preserve upstream warning policy; handwritten adapters are reviewed separately.
target_include_directories(cocor PRIVATE upstream/sources)

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
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME cocor_generation COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.py" $<TARGET_FILE:cocor>
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/frames")
  set_tests_properties(cocor_generation PROPERTIES TIMEOUT 60
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()
