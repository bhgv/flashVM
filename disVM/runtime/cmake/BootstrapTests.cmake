# Copyright (c) 2026 bhgv
# EN: Ordered BootstrapTests registrations; include from the runtime source directory.
# RU: Регистрация BootstrapTests; подключать из каталога исходников runtime.
if(DVM_RUNTIME_TESTS)
  enable_testing()
  foreach(mode jit interpreter)
    add_test(NAME disvm_tokenquery_example_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/tokenquery-demo.dis" ${mode})
    set_tests_properties(disvm_tokenquery_example_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()
if(DVM_RUNTIME_TESTS)
  get_target_property(sys_capture_includes disvm INCLUDE_DIRECTORIES)
  add_executable(disvm_sys_capture_test test_sys_capture.c sys_capture.c)
  add_dependencies(disvm_sys_capture_test disvm_runtime_sources)
  target_include_directories(disvm_sys_capture_test PRIVATE ${sys_capture_includes})
  target_compile_definitions(disvm_sys_capture_test PRIVATE DVM_NATIVE_REGISTERS=1)
  foreach(capture_case RANGE 0 18)
    add_test(NAME disvm_sys_capture_${capture_case} COMMAND disvm_sys_capture_test ${capture_case})
    set_tests_properties(disvm_sys_capture_${capture_case} PROPERTIES TIMEOUT 10)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
 enable_testing()
 include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Native64Review.cmake")
 add_test(NAME disvm_runtime_platform_selection COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_platform.py" "${CMAKE_COMMAND}"
   "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RuntimePlatform.cmake")
 foreach(test asinctasks asincviews idemdi idedebug idevectorpane idevector idepaint idetokenquery idesyntaxincremental idesyntax idecore limits coordinates find history replaceall utf8span ideworkspace idebridge idecomplete idebuiltins navigation viewport uicodec words indent decimalappend clipboard field stringslice values docprojection hybridtable tableguards tableleases)
  set(source "${root}/ide/dis/tests/${test}_test.b")
  if(test STREQUAL "docprojection")
    set(source "${root}/ide/dis/examples/document-projection/main.b")
  endif()
  if(test STREQUAL "tableleases")
    set(source "${root}/disVM/examples/table-leases/main.b")
  endif()
  if(test STREQUAL "tableguards")
    set(source "${root}/disVM/examples/table-guards/main.b")
  endif()
  if(test STREQUAL "hybridtable")
    set(source "${root}/disVM/examples/hybrid-table/main.b")
  endif()
  set(binary "${CMAKE_CURRENT_BINARY_DIR}/${test}-test.dis")
  add_custom_command(OUTPUT "${binary}"
   COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${binary}" "${source}"
   DEPENDS limbo-bootstrap "${source}" ${module_headers} VERBATIM)
  add_custom_target(disvm_native_${test} ALL DEPENDS "${binary}")
  # The unchanged million-character replace-all matrix takes about 55 s with
  # checked interpreter operands on this host; preserve its data and all oracles.
  set(application_budget 50)
  # Geometry regression includes multiple whole-page union/contour passes.
  if(test STREQUAL "idepaint")
    set(application_budget 120)
  endif()
  if(test STREQUAL "idesyntaxincremental")
    set(application_budget 120)
  endif()
  if(test STREQUAL "replaceall")
    set(application_budget 90)
  endif()
  math(EXPR application_outer_budget "${application_budget} + 10")
  foreach(mode jit interpreter)
   add_test(NAME disvm_application_${test}_${mode} COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>" "${binary}" "${mode}" "${application_budget}")
   set_tests_properties(disvm_application_${test}_${mode} PROPERTIES TIMEOUT ${application_outer_budget})
   if(test STREQUAL "tableleases")
     set(lease_mode --no-jit)
     if(mode STREQUAL "jit")
       set(lease_mode --jit)
     endif()
     add_test(NAME disvm_tableleases_gc_${mode} COMMAND "$<TARGET_FILE:disvm>"
       ${lease_mode} --quantum 1 --stats --max-instructions 20000 "${binary}")
     set_tests_properties(disvm_tableleases_gc_${mode} PROPERTIES TIMEOUT 15
       PASS_REGULAR_EXPRESSION "tables_live=0 keys_live=0")
   endif()
   if(test STREQUAL "replaceall")
     # This million-character matrix is CPU-heavy; preserve all functional checks.
     # RUN_SERIAL covers this CTest instance only: run the separate core/JIT
     # checkpoint before this runtime checkpoint to avoid cross-instance contention.
     set_tests_properties(disvm_application_${test}_${mode} PROPERTIES RUN_SERIAL TRUE)
   endif()
  endforeach()
 endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_asinctasks_quantum_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/asinctasks-test.dis" "${mode}" 50 1)
    set_tests_properties(disvm_asinctasks_quantum_${mode} PROPERTIES TIMEOUT 60)
    add_test(NAME disvm_asinctasks_limit_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_asinctasks_limit.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${root}/disVM/module" "${u}/module"
      "${root}/disVM/limbo/asinctasks.b" "${root}/ide/dis/tests/asinctaskslimit_test.b" "${mode}")
    set_tests_properties(disvm_asinctasks_limit_${mode} PROPERTIES TIMEOUT 130
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
  endforeach()
  add_test(NAME disvm_bigtable_boundary COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_bigtable_boundary.py" "$<TARGET_FILE:disvm>"
    "$<TARGET_FILE:limbo-bootstrap>" "${root}/disVM/module")
  set_tests_properties(disvm_bigtable_boundary PROPERTIES TIMEOUT 30
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
  foreach(mode jit interpreter)
    add_test(NAME disvm_decimal_append_example_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/decimal-append.dis" "${mode}")
    set_tests_properties(disvm_decimal_append_example_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  # Generate a classic module using the separate legacy compiler in the workspace.
  set(DVM_LEGACY_IMAGE "" CACHE FILEPATH "Optional classic Dis image for ABI rejection test")
  if(DVM_LEGACY_IMAGE)
    add_test(NAME disvm_application_input_contract COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_inputs.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${DVM_LEGACY_IMAGE}")
    set_tests_properties(disvm_application_input_contract PROPERTIES TIMEOUT 30)
  endif()
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_file_size_test test_file_size.c)
  target_link_libraries(disvm_file_size_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_file_extent_preserves_position COMMAND disvm_file_size_test
    "${CMAKE_CURRENT_BINARY_DIR}/file-size-test.bin")
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_module_byte_descriptors COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_byte_descriptor.py" "${u}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
  set_tests_properties(disvm_module_byte_descriptors PROPERTIES TIMEOUT 30
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_native_generation_idempotent COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_generation.py" "$<TARGET_FILE:limbo-bootstrap>" "${u}/libinterp" "${view}")
  set_tests_properties(disvm_native_generation_idempotent PROPERTIES TIMEOUT 30
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_editor_console_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_editor.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/editor-console.dis" "${mode}")
    set_tests_properties(disvm_editor_console_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_stdio_test test_stdio.c)
  target_link_libraries(disvm_stdio_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_standard_streams COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_stdio.py" "$<TARGET_FILE:disvm_stdio_test>")
  set_tests_properties(disvm_shared_standard_streams PROPERTIES TIMEOUT 30)
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_file_seek_test test_file_seek.c)
  target_link_libraries(disvm_file_seek_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_file_positioning COMMAND disvm_file_seek_test
    "${CMAKE_CURRENT_BINARY_DIR}/file-seek-test.bin")
  foreach(mode jit interpreter)
    add_test(NAME disvm_file_windows_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_file_windows.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${root}/ide/dis/bridge" "${mode}")
    set_tests_properties(disvm_file_windows_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_compiler_big_constant_pool COMMAND "${PYTHON_EXECUTABLE}"
    "${root}/tools/limbo-bootstrap/tests/big_constants.py" "$<TARGET_FILE:limbo-bootstrap>")
  set_tests_properties(disvm_compiler_big_constant_pool PROPERTIES TIMEOUT 30
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_resolver_test test_resolver.c)
  target_link_libraries(disvm_resolver_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_resolver COMMAND disvm_resolver_test)
  foreach(mode jit interpreter)
    add_test(NAME disvm_standard_modules_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/libraries-check.dis" "${mode}")
    add_test(NAME disvm_compiler_binary64_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_float_compiler.py" "$<TARGET_FILE:limbo-bootstrap>"
      "$<TARGET_FILE:disvm>" "${u}/module" "${mode}")
    set_tests_properties(disvm_standard_modules_${mode} disvm_compiler_binary64_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_nc64_tables_test test_native_tables.c)
  add_test(NAME disvm_nc64_table_layout_and_errors COMMAND disvm_nc64_tables_test)
endif()

# Explicit opt-in: core/console builds still need no fonts, rasterizer or window SDK.
option(DVM_EDITOR_SDL "Enable the real SDL2 editor window and native events" OFF)
