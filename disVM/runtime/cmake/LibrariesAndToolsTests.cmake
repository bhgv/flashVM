# Copyright (c) 2026 bhgv
# EN: Ordered LibrariesAndToolsTests registrations; include from the runtime source directory.
# RU: Регистрация LibrariesAndToolsTests; подключать из каталога исходников runtime.

# Preserve source scalar types and captured owners through native cyclic Dis.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_typed_loops_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/typed_loops.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_typed_loops_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# GC roots of private reference cells and captured payloads must survive every quantum.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_retained_loops_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/retained_loops.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_retained_loops_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Execute nonterminal conditions, surviving-arm initialization and scoped transfers.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_function_flow_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/function_flow.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_function_flow_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Definite reference stores and canonical nil roots survive GC at both quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_join_initialization_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/join_initialization.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_join_initialization_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Execute original/optimized private scalar cells under both dispatch modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_scalar_forwarding_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/scalar_forwarding.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_scalar_forwarding_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Check original/optimized scalar writes at both dispatch modes and quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_scalar_stores_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/scalar_stores.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_scalar_stores_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Execute Coco/R one-name traversal before/after optimization in both modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_value_iteration_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/value_iteration.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_value_iteration_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Actual selected Regex library covers the full Unicode scalar tag boundary.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_regex_unicode_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/regex-check.dis" "${mode}")
    set_tests_properties(disvm_regex_unicode_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

# Actual filesystem records and retained Readdir execute through OS-selected ports.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_sys_filesystem_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_filesystem.py" "$<TARGET_FILE:limbo-bootstrap>"
      "$<TARGET_FILE:disvm>" "${u}/module" "${root}/disVM/examples/filesystem/main.b"
      "${CMAKE_CURRENT_BINARY_DIR}/readdir.dis" "${mode}")
    set_tests_properties(disvm_sys_filesystem_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Standalone C89 port contract, independent of either VM heap.
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_filesystem_port_test test_filesystem_port.c)
  target_link_libraries(disvm_filesystem_port_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_filesystem COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_filesystem_port.py" "$<TARGET_FILE:disvm_filesystem_port_test>")
  set_tests_properties(disvm_shared_filesystem PROPERTIES TIMEOUT 60)
endif()

# Actual typed directory ownership and terminal-browser policy, no mocked Sys I/O.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_directory_browser_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_directory_browser.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${root}/ide/dis/bridge"
      "${root}/ide/dis/examples/directory-browser/main.b" "${CMAKE_CURRENT_BINARY_DIR}" "${mode}")
    set_tests_properties(disvm_directory_browser_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Graphical browser state lives in Dis; renderer sees only bounded copied labels.
if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_EDITOR)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_browser_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_browser.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${mode}")
    set_tests_properties(disvm_graphical_browser_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Optional executable tutorial uses the same generated native ABI and both real dispatch modes.
if(DVM_RUNTIME_NATIVE_DEMO)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${CMAKE_CURRENT_BINARY_DIR}/native-demo.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module" -I "${native_demo_root}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${native_demo_root}/main.b"
    DEPENDS limbo-bootstrap "${native_demo_root}/nativedemo.m" "${native_demo_root}/main.b" VERBATIM)
  add_custom_target(disvm_native_module_tutorial ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis")
  if(DVM_RUNTIME_TESTS)
    foreach(mode jit interpreter)
      add_test(NAME disvm_native_module_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_native_module.py" "$<TARGET_FILE:disvm>"
        "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${mode}")
      set_tests_properties(disvm_native_module_${mode} PROPERTIES TIMEOUT 60)
    endforeach()
  endif()
endif()

# Run the new source aliases through our FIR backend and actual native Dis dispatch.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_limbo_aliases_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/limbo_aliases.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_limbo_aliases_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Alias-typed string/key/table lifetimes must survive real native GC backedges.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_alias_owners_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/alias_owners.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_alias_owners_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Actual interpreter/native dispatch must agree on every signed32 bit boundary.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_int_bits_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/int_bits.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_int_bits_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Typed predicates must agree across actual native/interpreter dispatch and hosted tables.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_typed_predicates_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/typed_predicates.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_typed_predicates_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# IEEE ordered predicates must agree in actual native/interpreter and hosted Dis paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_real_conditions_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/real_conditions.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_real_conditions_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Exercise lazy numeric conditions in actual interpreter and native dispatch.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_numeric_logic_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/numeric_logic.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_numeric_logic_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Scalar cell updates, joins and numeric loops must execute on the actual VM.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_numeric_flow_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/numeric_flow.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_numeric_flow_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Service tutorial and independent multi-case acceptance use actual Dis modules.
if(DVM_RUNTIME_COMPILER)
  foreach(example compiler-service compiler-service-check build-panel artifact-export)
    set(source "${root}/ide/dis/examples/compiler-service/main.b")
    if(example STREQUAL "artifact-export")
      set(source "${root}/ide/dis/examples/artifact-export/main.b")
    endif()
    if(example STREQUAL "build-panel")
      set(source "${root}/ide/dis/examples/build-panel/main.b")
    endif()
    if(example STREQUAL "compiler-service-check")
      set(source "${root}/ide/dis/tests/compiler_service_test.b")
    endif()
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${example}.dis" "${CMAKE_CURRENT_BINARY_DIR}/${example}.sbl"
      COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/${example}.dis" "${source}"
      DEPENDS limbo-bootstrap "${source}" ${module_headers} VERBATIM)
    add_custom_target(disvm_${example} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${example}.dis")
  endforeach()
endif()

if(DVM_RUNTIME_COMPILER AND DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER AND DVM_FIR_SOURCE_COMPILER AND DVM_SWF_LINKER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_compiler_service_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_compiler_service.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/compiler-service-check.dis" ${mode}
      "${DVM_FIR_SOURCE_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_SWF_LINKER}")
    set_tests_properties(disvm_compiler_service_${mode} PROPERTIES TIMEOUT 120)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_compiler_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>"
      ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/compiler-service.dis")
    set_tests_properties(disvm_compiler_tutorial_${mode} PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "PASS compiler tutorial: revision=ok artifact=ok shutdown=ok")
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND NOT DVM_RUNTIME_COMPILER)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis"
      "${root}/ide/dis/tests/compiler_unavailable_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/compiler_unavailable_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_compiler_unavailable ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis")
  foreach(dispatch --jit --no-jit)
    add_test(NAME disvm_compiler_unavailable_${dispatch} COMMAND "$<TARGET_FILE:disvm>"
      ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis")
    set_tests_properties(disvm_compiler_unavailable_${dispatch} PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "PASS compiler unavailable: no native fallback, worker=nil, shutdown=ok")
  endforeach()
endif()

# The editor panel executes real native builds and a deterministic concurrent edit.
if(DVM_RUNTIME_COMPILER)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis"
      "${root}/ide/dis/tests/buildpanel_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/buildpanel_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_buildpanel_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis")
  if(DVM_RUNTIME_TESTS)
    foreach(mode jit interpreter)
      set(dispatch --jit)
      if(mode STREQUAL "interpreter")
        set(dispatch --no-jit)
      endif()
      add_test(NAME disvm_buildpanel_${mode} COMMAND "$<TARGET_FILE:disvm>"
        ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis")
      add_test(NAME disvm_buildpanel_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>"
        ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/build-panel.dis")
      set_tests_properties(disvm_buildpanel_tutorial_${mode} PROPERTIES TIMEOUT 30
        PASS_REGULAR_EXPRESSION "PASS build panel tutorial: revision=ok binary=ok debug=ok owners=closed")
      set_tests_properties(disvm_buildpanel_${mode} PROPERTIES TIMEOUT 60
        PASS_REGULAR_EXPRESSION "PASS compiler panel: targets=3 copies=ok UTF8=ok stale-native=reject shutdown=ok")
    endforeach()
  endif()
endif()

if(DVM_RUNTIME_COMPILER AND DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_build_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_build.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_graphical_build_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Binary artifact admission uses real channel workers, OS-exclusive files and CLI oracles.
if(DVM_RUNTIME_COMPILER)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis"
      "${root}/ide/dis/tests/artifact_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/artifact_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_artifact_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis")
  if(DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER AND DVM_FIR_SOURCE_COMPILER AND DVM_SWF_LINKER)
    foreach(mode jit interpreter)
      add_test(NAME disvm_artifact_export_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_artifact_export.py" "$<TARGET_FILE:disvm>"
        "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis" ${mode}
        "${DVM_FIR_SOURCE_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_SWF_LINKER}")
      set_tests_properties(disvm_artifact_export_${mode} PROPERTIES TIMEOUT 90)
    endforeach()
  endif()
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_exclusive_file_port COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_exclusive_file.py" "${DVM_FLASHVM_SOURCE_ROOT}/ports")
  set_tests_properties(disvm_exclusive_file_port PROPERTIES TIMEOUT 90)
endif()
if(DVM_RUNTIME_COMPILER AND DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_export_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_export.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_graphical_export_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
      "${root}/ide/dis/app/desktop.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/app/desktop.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_desktop_app ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -I "${root}/ide/dis/app"
      -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis" "${root}/ide/dis/examples/desktop/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop/main.b" "${root}/ide/dis/app/desktop.m" VERBATIM)
  add_custom_target(disvm_desktop_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis")
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
    -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis" "${root}/ide/dis/examples/desktop-lifecycle/main.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-lifecycle/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_lifecycle ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_lifecycle_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis")
    set_tests_properties(disvm_desktop_lifecycle_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop lifecycle tutorial:")
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis"
      "${root}/ide/dis/tests/desktop_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/desktop_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_desktop_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis")
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_owners_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 --max-instructions 2000000 "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis")
    set_tests_properties(disvm_desktop_owners_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop owners:")
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_movieclip_desktop_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_movieclip_desktop_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_pointer_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_pointer.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_pointer_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()
