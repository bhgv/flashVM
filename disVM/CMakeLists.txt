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
add_subdirectory(broker)
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
    control-transfer-native
    channel-capture-native
    cons-slots-native
    record-slots-native
    array-spans-native
    string-capture-native
    dis-numeric-literals
    decoded-addresses
    frame-allocation-native
    arm64-dispatch-compiler
    shift-slot-native
    branch-slot-native
    integer-branch-native
    numeric-slot-native
    fixed-slot-native
    array-slot-native
    mapped-preflight-native
    buffered-transfer-native
    jit-dispatch-native
    jit-dispatch-coverage
    arithmetic-slot-native
    transfer-slot-native
    integer-slot-native
    channel-queues
    channel-transfers
    dis-loader
    loader-snapshots
    loader-runtime
    pointer-maps
    gc-maps
    gc-arithmetic
    gc-idle
    readmod-io
    pc-target-boundary
    direct-target-native
    vm-widths
    dis-layout
    dis-control
    dis-rune
    dis-envelope
    dis-string-pool
    dis64-payloads
    dis64-pilot-native
    dis64-inspect
    dis-rune-case
    array-indices
    descriptor-index
    heap-bounds
    type-retains
    byte-array-retains
    list-retains
    channel-retains
    module-data-retains
    token-capacity
    token-retains
    clone-owners
    heap-clones
    heap-preflight
    reference-slots
    string-branches
    channels
    stack-lifecycle
    stack-spans
    exception-frames
    stack-unwind
    stack-startup
    call-slots
    module-imports
    return-slots
    return-layouts
    array-view-layouts
    array-copy-source
    conversion-sources
    raw-copy-layouts
    integer-format-layouts
    list-length-layouts
    real-format-layouts
    append-span-c
    append-layouts
    raise-slots
    raise-native
    string-slots
    sys-print
    sys-print-io
    sys-status-capture
    sys-query-capture
    sys-stream-directory-capture
    sys-multi-capture
    sys-pipe-types
    sys-print-spans
    string-owner-preflight
    string-codec-owners
    string-view-native
    sys-tokenize
    sys-print-public
    mapped-capture-native
    sys-utf-spans
    token-spans
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
    sys-file2chan-capture
    sys-tokenize-matrix
    sys-status-slots
    sys-stat-slots
    lists
    sys-pctl
    sys-pctl-capture
    emitter-layouts
    sys-packdir
    srv-results
    jit-unknown-kind
    mapped-copy-layouts
    decimal-layouts
    string-cell-layouts
    string-compare-layouts
    string-compare-spans-c
    scalar-view-spans-c
    byte-string-layouts
    array-create
    array-create-layouts
    amd64-emission-budget
    newzero-layouts
    scalar-cons-layouts
    payload-cons-layouts
    string-case-c
    string-bytes-layouts
    string-bytes-spans
    indc-preflight
    string-header-spans
    string-cell-spans-c
    utf8-view-spans-c
  )
    add_test(NAME disvm_${driver} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/${driver}.py")
    set_tests_properties(disvm_${driver} PROPERTIES TIMEOUT 120
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  endforeach()
  # Boundary arithmetic fixtures retain sanitizer checks in normal CTest runs.
  set_tests_properties(disvm_gc-arithmetic disvm_gc-idle disvm_readmod-io PROPERTIES
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -DNDEBUG -fsanitize=address,undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;ASAN_OPTIONS=detect_leaks=0")
  set_tests_properties(disvm_type-retains disvm_byte-array-retains disvm_list-retains disvm_channel-retains disvm_module-data-retains disvm_token-capacity disvm_token-retains disvm_clone-owners disvm_heap-clones disvm_heap-preflight disvm_loader-snapshots disvm_loader-runtime disvm_srv-results disvm_module-imports PROPERTIES
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  # Guard new byte-addressed Sys tuples and native argument bounds under UBSan.
  set_tests_properties(disvm_sys-status-slots disvm_sys-stat-slots PROPERTIES
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  set_tests_properties(disvm_sys-print-io disvm_sys-status-capture disvm_sys-query-capture
    disvm_sys-stream-directory-capture disvm_sys-multi-capture disvm_sys-pipe-types PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;FD_OWNER_FRAME=0;FD_OWNER_ARRAY_SHIFT=0")
  add_test(NAME disvm_dial_paths COMMAND "${Python3_EXECUTABLE}" "${u}/emu/port/tests/dial-paths.py")
  set_tests_properties(disvm_dial_paths PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  set_tests_properties(disvm_sys-file2chan-capture PROPERTIES TIMEOUT 180
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;FILE2CHAN_FRAME=0")
  set_tests_properties(disvm_sys-pctl-capture PROPERTIES TIMEOUT 180
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;PCTL_FRAME=0;PCTL_NODE=0")
  set_tests_properties(disvm_emitter-layouts PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME disvm_fixed_zero_div COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/fixed-zero-div.py")
  set_tests_properties(disvm_fixed_zero_div PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  # Retain production fixed-point selection and constant-folding regressions.
  foreach(driver fixed-ops fold-casts fold-strings case-cell-layouts asm-roundtrip optimizer-ranges node-chains node-queries node-transforms inline-graphs)
    add_test(NAME disvm_compiler_${driver} COMMAND "${Python3_EXECUTABLE}"
      "${u}/limbo/tests/${driver}.py")
    set_tests_properties(disvm_compiler_${driver} PROPERTIES TIMEOUT 120
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  endforeach()
  # The two production Str implementations share checked integer parser expressions.
  add_test(NAME disvm_string_radix_host COMMAND "${Python3_EXECUTABLE}"
    "${u}/appl/lib/tests/string-radix-host.py")
  set_tests_properties(disvm_string_radix_host PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  # Exercise bounded compiler source history and coordinate lookup in both word models.
  add_test(NAME disvm_source_history COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/source-history.py")
  set_tests_properties(disvm_source_history PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  if(backend STREQUAL "amd64")
    foreach(driver
      real-power-native
      real-to-integer-native
      real-branch-native
      goto-native
      owner-native
      list-slot-native
      mapped-copy-native
      decimal-native
      string-store-native
      string-append-native
      string-slice-native
      frame-native
      mframe-native
      return-native
      mcall-native
      call-return-native
      array-view-native
      raw-copy-native
      integer-format-native
      list-length-native
      real-format-native
      byte-string-native
      string-bytes-native
      array-reuse-native
      newzero-native
      scalar-cons-native
      payload-cons-native
      scalar-channel-native
      payload-channel-native
      pc-tables-native
      string-case-native
      mapped-cons-native
      typecheck-prefix
    )
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
    foreach(driver
      owner-jit
      string-owner-jit
      string-codec-owners-jit
      shift-slot-jit
      arithmetic-slot-jit
      branch-slot-jit
      integer-branch-jit
      numeric-slot-jit
      fixed-slot-jit
      array-slot-jit
      mapped-preflight-jit
      buffered-transfer-jit
      jit-dispatch-jit
      control-transfer-jit
      cons-slots-jit
      array-spans-jit
      string-capture-jit
      frame-allocation-jit
      mapped-copy-edges-jit
      raw-copy-edges-jit
      transfer-slot-jit
      integer-slot-jit
      list-slot-jit
      owner-optimized
      payload-channel-jit
      pc-tables-jit
      direct-target-jit
      string-case-jit
      mapped-cons-jit
      scalar-channel-jit
      payload-cons-jit
      scalar-cons-jit
      array-create-jit
      newzero-jit
      string-bytes-jit
      byte-string-jit
      scalar-view-spans-jit
      riscv-string-window
      raw-copy-jit
      integer-format-jit
      list-length-jit
      real-format-jit
      append-spans-jit
      decimal-spans-jit
      string-compare-jit
      indc-preflight-jit
      string-cell-spans-jit
      array-view-jit
      amd64-field-registers
      decimal-jit
      string-store-jit
      string-append-jit
      string-slice-jit
      frame-jit
      mframe-jit
      return-jit
      mcall-jit
      mapped-copy-jit
      module-transition-native
      spawn-preflight
    )
      add_test(NAME disvm_${driver} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/${driver}.py")
      set_tests_properties(disvm_${driver} PROPERTIES TIMEOUT 240
        ENVIRONMENT "${emulator_environment}")
    endforeach()
    foreach(arch amd64 arm64)
      add_test(NAME disvm_string-compare-jit_${arch} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/leaf64-jit.py")
      set_tests_properties(disvm_string-compare-jit_${arch} PROPERTIES TIMEOUT 120
        ENVIRONMENT "${emulator_environment};LEAF64_CASE=bytebranch;LEAF64_ARCH=${arch}")
    endforeach()
    # Verify low-owner rejection and the existing Type matrix on both priority ISAs.
    foreach(arch amd64 arm64)
      add_test(NAME disvm_typecheck_prefix_${arch} COMMAND "${Python3_EXECUTABLE}" "${u}/libinterp/tests/leaf64-jit.py")
      set_tests_properties(disvm_typecheck_prefix_${arch} PROPERTIES TIMEOUT 120
        ENVIRONMENT "${emulator_environment};LEAF64_CASE=typecheck;LEAF64_ARCH=${arch}")
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
  file(GLOB native_inputs "${u}/libinterp/*.c" "${u}/libinterp/*.h" "${u}/include/vmfetch.h" "${u}/include/vmchannel.h")
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

# The upstream per-command readiness report is reproducible and never rewritten by tests.
if(BUILD_TESTING)
  add_test(NAME disvm_jit_readiness COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/libinterp/tests/jit-readiness.py" --check)
  set_tests_properties(disvm_jit_readiness PROPERTIES TIMEOUT 120)
endif()


# The shared leaf compiler must reject truncated fast spans for ARM64 too.
if(BUILD_TESTING AND Python3_Interpreter_FOUND AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
  add_test(NAME disvm_arm64_cached_fast_spans COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/libinterp/tests/jit-dispatch-native.py")
  set_tests_properties(disvm_arm64_cached_fast_spans PROPERTIES TIMEOUT 120
    ENVIRONMENT "BRIDGE_BACKEND=arm64;BRIDGE_PREFLIGHT_ONLY=1;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# ALT/NBALT complete native tails must clean owned registrations after C errors.
if(BUILD_TESTING AND Python3_Interpreter_FOUND AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|aarch64|arm64|ARM64)$")
  add_test(NAME disvm_channel_queues_complete_jit COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/libinterp/tests/channel-queues.py")
  set_tests_properties(disvm_channel_queues_complete_jit PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-O1 -g -DNDEBUG -fsanitize=undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;QUEUE_JIT_ROUTE=1")
endif()

# Retained bootstrap optimizer graph/bit proofs use their actual selected source.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  foreach(driver optimizer-bits optimizer-graphs)
    add_test(NAME disvm_limbo_${driver} COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/upstream/limbo/tests/${driver}.py")
    set_tests_properties(disvm_limbo_${driver} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  endforeach()
endif()

# Exercise both emu/os native-long I/O boundaries without linking an OS runtime.
if(BUILD_TESTING)
  add_test(NAME disvm_native_io COMMAND "${Python3_EXECUTABLE}"
    "${u}/emu/port/tests/native-io.py")
  set_tests_properties(disvm_native_io PROPERTIES TIMEOUT 180
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;ASAN_OPTIONS=detect_leaks=0")
endif()

# Retained AOT reference is independent of the NC64 compiler-service route.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  add_test(NAME disvm_dis2c_native_slots COMMAND "${Python3_EXECUTABLE}"
    "${u}/utils/dis2c/tests/native-slots.py")
  add_test(NAME disvm_dis2c_reference COMMAND "${Python3_EXECUTABLE}" -O
    "${u}/utils/dis2c/tests/test.py")
  set_tests_properties(disvm_dis2c_native_slots disvm_dis2c_reference PROPERTIES TIMEOUT 180
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Shared Math primitive layouts now cover the retired host-only workaround.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  add_test(NAME disvm_math_local_types COMMAND "${Python3_EXECUTABLE}"
    "${u}/libinterp/tests/math-local-types.py")
  set_tests_properties(disvm_math_local_types PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# The shared executor validates every normal fetch before C/AOT/JIT callbacks.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  add_test(NAME disvm_xec_fetch_native COMMAND "${Python3_EXECUTABLE}"
    "${u}/libinterp/tests/xec-fetch-native.py")
  set_tests_properties(disvm_xec_fetch_native PROPERTIES TIMEOUT 180
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Execute the actual debugger, coverage and benchmark loops with their shared fetch.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  add_test(NAME disvm_instrumented_loops COMMAND "${Python3_EXECUTABLE}"
    "${u}/libinterp/tests/instrument-loops.py")
  set_tests_properties(disvm_instrumented_loops PROPERTIES TIMEOUT 240
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Native benchmark statistics and timer conversion from the pinned owning source.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  add_test(NAME disvm_bench_numeric COMMAND "${Python3_EXECUTABLE}"
    "${u}/libinterp/tests/bench-numeric.py")
  set_tests_properties(disvm_bench_numeric PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Actual ALT overlapping-publication matrices, C and native RX dispatch.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  foreach(route c rx)
    add_test(NAME disvm_alt_publication_${route} COMMAND "${Python3_EXECUTABLE}"
      "${u}/libinterp/tests/channel-queues.py")
    set(route_environment "QUEUE_JIT_ROUTE=0")
    if(route STREQUAL "rx")
      set(route_environment "QUEUE_JIT_ROUTE=1")
    endif()
    set_tests_properties(disvm_alt_publication_${route} PROPERTIES TIMEOUT 240
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;CFLAGS=-O1 -g -DNDEBUG -fsanitize=address,undefined -fno-sanitize-recover=all;QUEUE_CASES=publication-bad-ready publication-bad-wake publication-bad-output publication-idle publication-wake publication-wake-tuple publication-result publication-payload publication-tuple;${route_environment}")
  endforeach()
endif()

# Exercise native entry REG borrowing and temporary receive owners with the real
# emu/os entry bodies. Error injection uses UBSan; nonthrowing cases also use ASan.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  foreach(profile errors nonthrowing)
    add_test(NAME disvm_native_channel_entry_${profile} COMMAND "${Python3_EXECUTABLE}"
      "${u}/libinterp/tests/channel-transfers.py")
    set(entry_flags "-O1 -g -DNDEBUG -fsanitize=undefined -fno-sanitize-recover=all")
    set(entry_selection "")
    if(profile STREQUAL "nonthrowing")
      set(entry_flags "-O1 -g -DNDEBUG -fsanitize=address,undefined -fno-sanitize-recover=all")
      set(entry_selection "CHANNEL_TRANSFER_CASES=entry-move entry-allocate entry-destroy entry-block entry-refill;ENTRY_MODE=1")
    endif()
    set_tests_properties(disvm_native_channel_entry_${profile} PROPERTIES TIMEOUT 240
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;CFLAGS=${entry_flags};${entry_selection}")
  endforeach()
endif()

# Real queue append/cancel/dequeue chains, including cycles and damaged tails.
# Use UBSan for the full injected-error matrix; retain the upstream error frames.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  foreach(route c rx)
    set(chain_route "QUEUE_JIT_ROUTE=0")
    if(route STREQUAL "rx")
      set(chain_route "QUEUE_JIT_ROUTE=1")
    endif()
    add_test(NAME disvm_queue_chain_${route} COMMAND "${Python3_EXECUTABLE}"
      "${u}/libinterp/tests/channel-queues.py")
    set_tests_properties(disvm_queue_chain_${route} PROPERTIES TIMEOUT 240
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;CFLAGS=-O1 -g -DNDEBUG -fsanitize=undefined -fno-sanitize-recover=all;QUEUE_CASES=queue-chains queue-slots queue-pop queue-long;${chain_route}")
  endforeach()
endif()

# ALT must preflight every guard before registration, then survive drained queues
# during finalization. Exercise authoritative C bodies and native AMD64 RX tails.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  foreach(family metadata hangup)
    set(alt_cases "alt-metadata")
    if(family STREQUAL "hangup")
      set(alt_cases "alt-hangup queue-hangup")
    endif()
    foreach(route c rx)
      set(alt_route "QUEUE_JIT_ROUTE=0")
      if(route STREQUAL "rx")
        set(alt_route "QUEUE_JIT_ROUTE=1")
      endif()
      add_test(NAME disvm_alt_${family}_${route} COMMAND "${Python3_EXECUTABLE}"
        "${u}/libinterp/tests/channel-queues.py")
      set_tests_properties(disvm_alt_${family}_${route} PROPERTIES TIMEOUT 240
        ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;CFLAGS=-O1 -g -DNDEBUG -fsanitize=undefined -fno-sanitize-recover=all;QUEUE_CASES=${alt_cases};${alt_route}")
    endforeach()
  endforeach()
endif()

# Destructor callbacks may borrow the decoded register set. Execute the complete
# owner/return/extent matrices with active predicates under UBSan and NDEBUG.
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
  foreach(driver stack-unwind return-native stack-lifecycle)
    add_test(NAME disvm_stack_callbacks_${driver} COMMAND "${Python3_EXECUTABLE}"
      "${u}/libinterp/tests/${driver}.py")
    set_tests_properties(disvm_stack_callbacks_${driver} PROPERTIES TIMEOUT 240
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;CFLAGS=-O1 -g -DNDEBUG -fsanitize=undefined -fno-sanitize-recover=all")
  endforeach()
endif()

# The active Limbo writers share the selected unsigned-shift encoder header.
if(BUILD_TESTING)
  add_test(NAME disvm_dis-constants COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/dis-constants.py")
endif()

# Independent FlashVM oracle uses the selected upstream helper's actual C API.
if(BUILD_TESTING)
  add_test(NAME disvm_rune_order_crosscheck COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/rune_order_crosscheck.py" "${u}")
endif()

# Shared structural codecs and standalone tools do not enable a new VM ABI.
add_subdirectory(envelope)
