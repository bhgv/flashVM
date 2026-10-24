# Independent selected Dis/JIT core and explicit legacy bootstrap from owned sources.
cmake_minimum_required(VERSION 3.10)
project(disvm VERSION 0.1.2 LANGUAGES C)
if(NOT UNIX)
  message(FATAL_ERROR "This initial host build supports POSIX; native OS adapters remain separate")
endif()
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(u "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
set(host_include "${CMAKE_CURRENT_SOURCE_DIR}/host/include")
# The upstream lib9 header is shared POSIX C declarations; ARM assembly is never built.
set(sdk_include "${host_include}" "${u}/Linux/arm/include" "${u}/include" "${u}/libinterp")

# This archive is the genuine interpreter core. OS/kernel host functions are
# intentionally unresolved until an embedding host implements kernel.h.
add_library(disvm_core STATIC
  upstream/libinterp/alt.c
  upstream/libinterp/conv.c
  upstream/libinterp/dec.c
  upstream/libinterp/disc.c
  upstream/libinterp/gc.c
  upstream/libinterp/heap.c
  upstream/libinterp/heapaudit.c
  upstream/libinterp/link.c
  upstream/libinterp/load.c
  upstream/libinterp/loader.c
  upstream/libinterp/raise.c
  upstream/libinterp/readmod.c
  upstream/libinterp/runt.c
  upstream/libinterp/stack.c
  upstream/libinterp/string.c
  upstream/libinterp/validstk.c
  upstream/libinterp/xec.c
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(disvm_core PRIVATE -fno-strict-aliasing)
endif()
target_include_directories(disvm_core PUBLIC ${sdk_include} PRIVATE "${u}/emu/port")
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" cpu)
if(cpu MATCHES "^(x86_64|amd64)$")
  set(backend amd64)
elseif(cpu MATCHES "^(aarch64|arm64)$")
  set(backend arm64)
elseif(cpu MATCHES "^riscv64")
  set(backend riscv64)
else()
  message(FATAL_ERROR "No accepted host backend for ${CMAKE_SYSTEM_PROCESSOR}; retained 386 sources are reference-only")
endif()
target_sources(disvm_core PRIVATE "${u}/libinterp/comp-${backend}.c")
message(STATUS "Dis/JIT core backend: ${backend}; host scheduler/embedding is not supplied by this archive")
# Native image transport is independent of kernel.h; the parser adapter retains
# the upstream VM's host requirements instead of inventing a second parser.
add_library(disvm_module_image STATIC host/module_image.c)
target_include_directories(disvm_module_image PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(disvm_module_parser STATIC host/module_parse.c)
target_include_directories(disvm_module_parser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE ${sdk_include})
target_link_libraries(disvm_module_parser PUBLIC disvm_module_image disvm_core)
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
  add_test(NAME disvm_memory_parser_adapter COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/module_parser.py")
  set_tests_properties(disvm_memory_parser_adapter PROPERTIES TIMEOUT 120)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp")
  foreach(driver integer-ops channel-queues channel-transfers dis-loader pointer-maps vm-widths dis-layout array-indices descriptor-index heap-bounds reference-slots string-branches channels)
    add_test(NAME disvm_${driver} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/${driver}.py")
    set_tests_properties(disvm_${driver} PROPERTIES TIMEOUT 120
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  endforeach()
  if(backend STREQUAL "amd64")
    foreach(bits 32 64)
      add_test(NAME disvm_native_jit_word${bits} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/leaf64-runtime.py")
      set_tests_properties(disvm_native_jit_word${bits} PROPERTIES TIMEOUT 120
        ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;LEAF_WORD_BITS=${bits}")
    endforeach()
  endif()
endif()

# Experimental opt-in only: TLS registers do not by themselves synchronize the
# remaining global heaps/modules/GC/scheduler. Do not run a complete shared VM yet.
option(DVM_NATIVE_REGISTERS "Compile interpreter references to R through native TLS" OFF)
if(DVM_NATIVE_REGISTERS)
  add_subdirectory(host shared-host)
  find_package(PythonInterp 3 REQUIRED)
  set(native_dir "${CMAKE_CURRENT_BINARY_DIR}/native-registers")
  execute_process(COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/host/prepare_native_registers.py"
    --source "${u}/libinterp" --output "${native_dir}" RESULT_VARIABLE native_status)
  if(NOT native_status EQUAL 0)
    message(FATAL_ERROR "Native register view generation failed")
  endif()
  get_target_property(original_sources disvm_core SOURCES)
  set(native_sources)
  foreach(source IN LISTS original_sources)
    get_filename_component(name "${source}" NAME)
    list(APPEND native_sources "${native_dir}/${name}")
  endforeach()
  set_property(TARGET disvm_core PROPERTY SOURCES "${native_sources}")
  file(GLOB native_inputs "${u}/libinterp/*.c" "${u}/libinterp/*.h")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${native_inputs}
    "${CMAKE_CURRENT_SOURCE_DIR}/host/prepare_native_registers.py")
  target_compile_definitions(disvm_core PUBLIC DVM_NATIVE_REGISTERS=1)
  target_sources(disvm_core PRIVATE host/native_registers.c)
  target_include_directories(disvm_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(disvm_core PUBLIC disvm_flashvm_host)
endif()
