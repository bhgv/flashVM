cmake_minimum_required(VERSION 3.10)
# This tool also builds independently; embedding parents already set C99/Python.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(dis64_envelope VERSION 0.3.0 LANGUAGES C)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED ON)
  set(CMAKE_C_EXTENSIONS OFF)
  include(CTest)
  if(BUILD_TESTING)
    if(CMAKE_VERSION VERSION_LESS "3.12")
      find_package(PythonInterp 3 REQUIRED)
      set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    else()
      find_package(Python3 COMPONENTS Interpreter REQUIRED)
    endif()
  endif()
endif()

# Shared header-only C99 codec and the same upstream standalone inspector.
get_filename_component(envelope_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
add_library(disvm_envelope INTERFACE)
target_include_directories(disvm_envelope INTERFACE "${envelope_root}/upstream/include")
add_executable(dis64inspect "${envelope_root}/upstream/utils/dis64inspect/dis64inspect.c")
target_link_libraries(dis64inspect PRIVATE disvm_envelope)
add_executable(dis64_envelope_example examples/main.c)
target_link_libraries(dis64_envelope_example PRIVATE disvm_envelope)
add_executable(dis64_interfaces_example examples/interfaces.c)
target_link_libraries(dis64_interfaces_example PRIVATE disvm_envelope)
add_executable(dis64_pilot_example examples/pilot.c)
target_link_libraries(dis64_pilot_example PRIVATE disvm_envelope)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(dis64_interfaces_example PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-function)
  target_compile_options(dis64_pilot_example PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-function)
  target_compile_options(dis64inspect PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-function)
  target_compile_options(dis64_envelope_example PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
set(envelope_sample "${CMAKE_CURRENT_BINARY_DIR}/sample.dis64")
# A cross build must not execute its target program on the build host.
if(NOT CMAKE_CROSSCOMPILING)
  add_custom_command(OUTPUT "${envelope_sample}"
    COMMAND "$<TARGET_FILE:dis64_envelope_example>" "${envelope_sample}"
    DEPENDS dis64_envelope_example VERBATIM)
  add_custom_target(dis64_envelope_sample ALL DEPENDS "${envelope_sample}")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pilot42.dis64"
    COMMAND "$<TARGET_FILE:dis64_pilot_example>" "${CMAKE_CURRENT_BINARY_DIR}/pilot42.dis64"
    DEPENDS dis64_pilot_example VERBATIM)
  add_custom_target(dis64_pilot_sample ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/pilot42.dis64")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pilot-init42.dis64"
    COMMAND "$<TARGET_FILE:dis64_pilot_example>" "${CMAKE_CURRENT_BINARY_DIR}/pilot-init42.dis64" --initialized
    DEPENDS dis64_pilot_example VERBATIM)
  add_custom_target(dis64_storage_sample ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/pilot-init42.dis64")
endif()

# Both parent configurations already resolve Python for their host checks.
if(NOT CMAKE_CROSSCOMPILING AND (BUILD_TESTING OR DVM_RUNTIME_TESTS) AND (Python3_EXECUTABLE OR PYTHON_EXECUTABLE))
  if(Python3_EXECUTABLE)
    set(envelope_python "${Python3_EXECUTABLE}")
  else()
    set(envelope_python "${PYTHON_EXECUTABLE}")
  endif()
  add_test(NAME dis64_envelope_cli COMMAND "${envelope_python}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/check.py" "$<TARGET_FILE:dis64inspect>"
    "$<TARGET_FILE:dis64_envelope_example>"
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/sample.dis64.hex")
endif()

if(NOT CMAKE_CROSSCOMPILING AND envelope_python)
  add_test(NAME dis64_interfaces_cli COMMAND "$<TARGET_FILE:dis64_interfaces_example>")
  add_test(NAME dis64_pilot_cli COMMAND "${envelope_python}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/pilot.py" "$<TARGET_FILE:dis64inspect>"
    "$<TARGET_FILE:dis64_pilot_example>"
    "${envelope_root}/upstream/libinterp/tests/dis64/pilot42.hex")
endif()
