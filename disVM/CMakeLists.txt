# Independent selected Dis/JIT core and explicit legacy bootstrap from owned sources.
cmake_minimum_required(VERSION 3.10)
project(disvm VERSION 0.1.6 LANGUAGES C)
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
  # Selected production regressions and their manifest-owned fixture dependencies.
  foreach(driver
    integer-ops
    channel-queues
    channel-transfers
    dis-loader
    pointer-maps
    vm-widths
    dis-layout
    array-indices
    descriptor-index
    heap-bounds
    reference-slots
    string-branches
    channels
    stack-lifecycle
    exception-frames
    stack-unwind
    stack-startup
    call-slots
    return-slots
    raise-slots
    string-slots
    sys-print
    sys-tokenize
    sys-utf
    vm-addresses
    fixed-coeff
    fixed-frame
    jit-coverage
    heap-query
    prog-diagnostics
    sys-fd-owners
    sys-io
    sys-stream
    sys-dirread
    sys-file2chan
    sys-tokenize-matrix
    sys-status-slots
    sys-stat-slots
    lists
    sys-pctl
    sys-packdir
    srv-results
    jit-unknown-kind
    mapped-copy-layouts
  )
    add_test(NAME disvm_${driver} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/${driver}.py")
    set_tests_properties(disvm_${driver} PROPERTIES TIMEOUT 120
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  endforeach()
  # Guard new byte-addressed Sys tuples and native argument bounds under UBSan.
  set_tests_properties(disvm_sys-status-slots disvm_sys-stat-slots PROPERTIES
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME disvm_fixed_zero_div COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/fixed-zero-div.py")
  set_tests_properties(disvm_fixed_zero_div PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  if(backend STREQUAL "amd64")
    foreach(driver real-power-native real-to-integer-native real-branch-native goto-native owner-native mapped-copy-native decimal-native string-store-native string-append-native)
      add_test(NAME disvm_${driver} COMMAND "${Python3_EXECUTABLE}"
        "${u}/libinterp/tests/${driver}.py")
      set_tests_properties(disvm_${driver} PROPERTIES TIMEOUT 120
        ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
    endforeach()
    add_test(NAME disvm_decimal-vm-parser COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/decimal-native.py")
    set_tests_properties(disvm_decimal-vm-parser PROPERTIES TIMEOUT 120
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;DECIMAL_VM_PARSER=1;DECIMAL_ACCEPTED_ONLY=1")
    add_test(NAME disvm_string-compare-native COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/string-branches.py")
    set_tests_properties(disvm_string-compare-native PROPERTIES TIMEOUT 120
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;STRING_BRANCH_NATIVE=1")
    foreach(bits 32 64)
      add_test(NAME disvm_native_jit_word${bits} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/leaf64-runtime.py")
      set_tests_properties(disvm_native_jit_word${bits} PROPERTIES TIMEOUT 120
        ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;LEAF_WORD_BITS=${bits}")
    endforeach()
  endif()
  set(DVM_EMULATOR_PYTHONPATH "" CACHE PATH "Optional explicit capstone/unicorn test dependency directory")
  set(emulator_environment "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  if(DVM_EMULATOR_PYTHONPATH)
    list(APPEND emulator_environment "PYTHONPATH=${DVM_EMULATOR_PYTHONPATH}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=${DVM_EMULATOR_PYTHONPATH}"
      "${Python3_EXECUTABLE}" -c "import capstone, unicorn"
      RESULT_VARIABLE have_instruction_emulators OUTPUT_QUIET ERROR_QUIET)
  else()
    execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import capstone, unicorn"
      RESULT_VARIABLE have_instruction_emulators OUTPUT_QUIET ERROR_QUIET)
  endif()
  if(have_instruction_emulators EQUAL 0)
    foreach(driver decimal-jit string-store-jit string-append-jit)
      add_test(NAME disvm_${driver} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/${driver}.py")
      set_tests_properties(disvm_${driver} PROPERTIES TIMEOUT 240
        ENVIRONMENT "${emulator_environment}")
    endforeach()
    foreach(arch amd64 arm64)
      add_test(NAME disvm_string-compare-jit_${arch} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/leaf64-jit.py")
      set_tests_properties(disvm_string-compare-jit_${arch} PROPERTIES TIMEOUT 120
        ENVIRONMENT "${emulator_environment};LEAF64_CASE=bytebranch;LEAF64_ARCH=${arch}")
    endforeach()
    add_test(NAME disvm_string-read-jit COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/leaf64-jit.py")
    set_tests_properties(disvm_string-read-jit PROPERTIES TIMEOUT 120
      ENVIRONMENT "${emulator_environment};LEAF64_CASE=character")
  else()
    message(STATUS "Optional string-read/decimal/string-store instruction emulation needs capstone and unicorn")
  endif()
  # Compile production foreign emitters on this host; these fixtures never execute
  # ARM64/RV64 machine code and must not be reported as native platform acceptance.
  foreach(arch arm64 riscv64)
    foreach(bits 32 64)
      if(arch STREQUAL "riscv64")
        set(profiles i zmmul m)
      else()
        set(profiles native)
      endif()
      foreach(profile IN LISTS profiles)
        set(check_name "disvm_emitter_${arch}_${profile}_word${bits}")
        add_test(NAME "${check_name}" COMMAND "${Python3_EXECUTABLE}"
          "${u}/libinterp/tests/leaf64-runtime.py")
        set_tests_properties("${check_name}" PROPERTIES TIMEOUT 120
          ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;LEAF_RUNTIME_BACKEND=${arch};LEAF_WORD_BITS=${bits};RV_FEATURE_PROFILE=${profile}")
      endforeach()
    endforeach()
  endforeach()
endif()

# Installable diagnostics give the core a runnable acceptance frontend without
# pretending that the remaining general-purpose host/ABI work is complete.
option(DVM_BUILD_DIAGNOSTICS "Build native AMD64 Dis/JIT check programs" ON)
if(DVM_BUILD_DIAGNOSTICS AND backend STREQUAL "amd64" AND NOT CMAKE_CROSSCOMPILING
    AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
  find_package(PythonInterp 3 REQUIRED)
  file(GLOB diagnostic_inputs "${u}/libinterp/*.c" "${u}/libinterp/*.h"
    "${u}/include/*.h" "${u}/libinterp/tests/*.c")
  foreach(bits 32 64)
    set(probe "${CMAKE_CURRENT_BINARY_DIR}/disvm-jit-word${bits}")
    add_custom_command(OUTPUT "${probe}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/host/build_probe.py"
        --driver "${u}/libinterp/tests/leaf64-runtime.py" --output "${probe}"
        --cc "${CMAKE_C_COMPILER}" --wordbits ${bits}
      DEPENDS host/build_probe.py "${u}/libinterp/tests/leaf64-runtime.py" ${diagnostic_inputs}
      VERBATIM)
    add_custom_target(disvm_diagnostic_word${bits} ALL DEPENDS "${probe}")
    if(BUILD_TESTING)
      add_test(NAME disvm_installed_diagnostic_word${bits} COMMAND "${probe}")
      set_tests_properties(disvm_installed_diagnostic_word${bits} PROPERTIES TIMEOUT 30)
    endif()
  endforeach()
  configure_file(host/disvm-check.sh "${CMAKE_CURRENT_BINARY_DIR}/disvm-check" COPYONLY)
  execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
    "import os,sys;os.chmod(sys.argv[1],493)" "${CMAKE_CURRENT_BINARY_DIR}/disvm-check")
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
  # Explicit byte-array boundary; callers retain the production Dis scheduler/GC owner.
  add_library(disvm_ui_dis_channel STATIC host/dis_ui_channel.c)
  target_include_directories(disvm_ui_dis_channel PRIVATE ${sdk_include}
    "${DVM_FLASHVM_SOURCE_ROOT}/include")
  target_link_libraries(disvm_ui_dis_channel PUBLIC disvm_core disvm_flashvm_host)
  if(BUILD_TESTING AND DVM_FLASHVM_LIBRARIES AND NOT CMAKE_CROSSCOMPILING
      AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
    # The extraction fixture owns a legacy global REG; never link it to the TLS production object.
    add_library(disvm_ui_dis_fixture STATIC host/dis_ui_channel.c)
    target_include_directories(disvm_ui_dis_fixture PRIVATE ${sdk_include}
      "${CMAKE_CURRENT_SOURCE_DIR}/include" "${DVM_FLASHVM_SOURCE_ROOT}/include")
    add_test(NAME disvm_ui_dis_channels COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/dis_ui_channel.py"
      --flashvm-root "${DVM_FLASHVM_SOURCE_ROOT}"
      --adapter "$<TARGET_FILE:disvm_ui_dis_fixture>"
      --host "$<TARGET_FILE:disvm_flashvm_host>"
      --libraries "${DVM_FLASHVM_LIBRARIES}")
    set_tests_properties(disvm_ui_dis_channels PROPERTIES TIMEOUT 120
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  endif()

endif()

# Optional source-built bootstrap; no hidden downloaded SDK or deployed Python runtime.
set(DVM_LIMBO_EXECUTABLE "" CACHE FILEPATH "Explicit Limbo compiler for bridge modules")
if(DVM_LIMBO_EXECUTABLE)
  if(NOT EXISTS "${DVM_LIMBO_EXECUTABLE}")
    message(FATAL_ERROR "DVM_LIMBO_EXECUTABLE does not exist")
  endif()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/uibridge.dis"
    COMMAND "${DVM_LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}/module"
      -o "${CMAKE_CURRENT_BINARY_DIR}/uibridge.dis" "${CMAKE_CURRENT_SOURCE_DIR}/limbo/uibridge.b"
    DEPENDS module/uibridge.m limbo/uibridge.b "${DVM_LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/uiecho.dis"
    COMMAND "${DVM_LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}/module"
      -I "${CMAKE_CURRENT_SOURCE_DIR}/examples/ui-channel"
      -o "${CMAKE_CURRENT_BINARY_DIR}/uiecho.dis" "${CMAKE_CURRENT_SOURCE_DIR}/examples/ui-channel/uiecho.b"
    DEPENDS module/uibridge.m examples/ui-channel/uiecho.m examples/ui-channel/uiecho.b
      "${DVM_LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/console-check.dis"
      "${CMAKE_CURRENT_BINARY_DIR}/console-check.sbl"
    COMMAND "${DVM_LIMBO_EXECUTABLE}" -g -I "${u}/module"
      -o "${CMAKE_CURRENT_BINARY_DIR}/console-check.dis"
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/console-check/main.b"
    DEPENDS examples/console-check/main.b "${u}/module/sys.m" "${DVM_LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(disvm_console_example ALL DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/console-check.dis" "${CMAKE_CURRENT_BINARY_DIR}/console-check.sbl")
  add_custom_target(disvm_ui_modules ALL DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/uibridge.dis" "${CMAKE_CURRENT_BINARY_DIR}/uiecho.dis")
endif()
