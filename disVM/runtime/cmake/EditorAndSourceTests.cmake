# Copyright (c) 2026 bhgv
# EN: Ordered EditorAndSourceTests registrations; include from the runtime source directory.
# RU: Регистрация EditorAndSourceTests; подключать из каталога исходников runtime.

set(DVM_EDITOR_TEST_FONT "" CACHE FILEPATH "Optional explicit font override for graphical editor checks")
if(DVM_RUNTIME_EDITOR AND NOT DVM_EDITOR_TEST_FONT)
  set(DVM_EDITOR_TEST_FONT "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf")
endif()
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_editor_async_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_async_editor.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_editor_async_${mode} PROPERTIES TIMEOUT 60)
    add_test(NAME disvm_graphical_editor_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_editor.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_graphical_editor_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_ide_application_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_editor.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_ide_application_${mode} PROPERTIES TIMEOUT 120)

    if(DVM_EDITOR_SDL)
      add_test(NAME disvm_graphical_sdl_upload_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_editor.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}" window)
      set_tests_properties(disvm_graphical_sdl_upload_${mode} PROPERTIES TIMEOUT 120
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    endif()
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_actions_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_actions.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_graphical_actions_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_location_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_location.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_graphical_location_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_default_font_zoom_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_zoom.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_default_font_zoom_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_caret_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_caret.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_caret_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_fields_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_fields.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_fields_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_field_clipboard_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_field_clipboard.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_field_clipboard_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_resize_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_resize.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_resize_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_clipboard_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_clipboard.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_clipboard_${mode} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
    if(DVM_EDITOR_SDL)
      add_test(NAME disvm_graphical_field_clipboard_sdl_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_field_clipboard.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}" sdl)
      set_tests_properties(disvm_graphical_field_clipboard_sdl_${mode} PROPERTIES TIMEOUT 120
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
      add_test(NAME disvm_graphical_clipboard_sdl_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_clipboard.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}" sdl)
      set_tests_properties(disvm_graphical_clipboard_sdl_${mode} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
      add_test(NAME disvm_graphical_resize_sdl_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_resize.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}" window)
      set_tests_properties(disvm_graphical_resize_sdl_${mode} PROPERTIES TIMEOUT 120
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    endif()
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  # The long-line editor uses this public path transaction; reuse its actual allocation oracle under UBSan.
  add_executable(disvm_editor_path_transaction "${root}/flashVM/tests/path_append_transaction.cpp")
  target_link_libraries(disvm_editor_path_transaction PRIVATE flashvm)
  add_test(NAME disvm_editor_path_transaction COMMAND disvm_editor_path_transaction)
  set_tests_properties(disvm_editor_path_transaction PROPERTIES TIMEOUT 30)
endif()

# Optional cross-project acceptance never makes the standalone runtime depend on the FIR build.
set(DVM_FIR_COMPILER "" CACHE FILEPATH "Our optional FIR compiler for direct Dis execution tests")
set(DVM_FIR_SOURCE_COMPILER "" CACHE FILEPATH "Our optional Coco/R numeric source compiler")
if(DVM_RUNTIME_TESTS AND EXISTS "${DVM_FIR_COMPILER}" AND EXISTS "${DVM_FIR_SOURCE_COMPILER}")
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_tables_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_tables.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_tables_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_big_shifts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/big_shifts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_big_shifts_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_table_cfg_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_cfg.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_cfg_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_big_decisions_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/big_decisions.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_big_decisions_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_big_cfg_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_big_cfg.py" "${DVM_FIR_COMPILER}"
      "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_big_cfg_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_dis_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_dis.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_dis_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_cfg_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_cfg.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_cfg_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_reals_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_reals.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_reals_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_limbo_reals_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_limbo_reals.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${mode}")
    set_tests_properties(disvm_limbo_reals_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Real projection owner processes; do not confuse this with a network RPC test.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_projection_inspector_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/projection-check.dis" "${mode}")
    set_tests_properties(disvm_projection_inspector_${mode} PROPERTIES TIMEOUT 30)
  endforeach()
endif()

# Optional explicit development tools check the shipped AS client, not a deployed runtime dependency.
set(DVM_IDE_NODE "" CACHE FILEPATH "Node for optional AS client contract checks")
set(DVM_IDE_AS2 "" CACHE FILEPATH "Our Coco/R AS2 syntax frontend for optional client checks")
if(DVM_RUNTIME_TESTS AND DVM_IDE_NODE)
  add_test(NAME disvm_projection_as_client COMMAND "${DVM_IDE_NODE}"
    "${root}/ide/as/test_projection_values.js" "${root}/ide/as/projection-values.as")
endif()
if(DVM_RUNTIME_TESTS AND DVM_IDE_AS2)
  add_test(NAME disvm_projection_as_syntax COMMAND "${DVM_IDE_AS2}" "${root}/ide/as/projection-values.as")
endif()

# The watch panel uses real Dis adapter channels and actual FlashVM raster output, without SDL.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_projection_inspector_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_inspector.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${mode}")
    set_tests_properties(disvm_graphical_projection_inspector_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(feature gutter close)
    foreach(mode jit interpreter)
      add_test(NAME disvm_graphical_${feature}_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_${feature}.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${mode}")
      set_tests_properties(disvm_graphical_${feature}_${mode} PROPERTIES TIMEOUT 120)
    endforeach()
  endforeach()
endif()

# Compile and execute new Limbo table operations on the actual matching DisVM.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_mutation_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_mutation.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_mutation_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Arm-local owners and assignments execute on both actual dispatch paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_arms_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_arms.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_arms_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Run comparison values and eager checked operands on both actual VM dispatch modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_comparisons_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_comparisons.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_comparisons_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Short-circuit joins use actual scalar frame cells on both VM dispatch paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_short_circuit_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/short_circuit.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_short_circuit_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Compound big/table updates execute on both real VM dispatch paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_compound_tables_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/compound_tables.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_compound_tables_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Native Unicode string instructions execute with roots live across one-step GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_unicode_strings_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/unicode_strings.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_unicode_strings_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Native-word execution must retain int32 semantics in both actual VM dispatch modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_int_arithmetic_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/int_arithmetic.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_int_arithmetic_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Rooted concatenations and int bit patterns survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_text_bits_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/text_bits.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_text_bits_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Checked shifts and signed32 results survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_int_shifts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/int_shifts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_int_shifts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Ordered strings and checked int casts survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_text_values_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/text_values.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_text_values_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Complete JIT tails and the direct hot route must share NC64 table/counter policy.
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_native_leaf_test test_native_leaf.c)
  target_include_directories(disvm_native_leaf_test PRIVATE "${u}/include")
  target_link_libraries(disvm_native_leaf_test PRIVATE Threads::Threads)
  add_test(NAME disvm_complete_tail_nc64_policy COMMAND disvm_native_leaf_test)
endif()

# Ordered constructor fields and key snapshots survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_literals_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_literals.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_literals_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Relative sequence indexes and copies survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_relative_sequences_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/relative_sequences.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_relative_sequences_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Reference-compiled native arrays cover every indexed element width and mapped GC owners.
if(DVM_RUNTIME_TESTS)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module" -o "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis"
      "${root}/disVM/examples/relative-sequences/main.b"
    DEPENDS limbo-bootstrap "${root}/disVM/examples/relative-sequences/main.b" VERBATIM)
  add_custom_target(disvm_relative_arrays ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis")
  foreach(mode jit interpreter)
    add_test(NAME disvm_relative_arrays_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis" "${mode}")
    set_tests_properties(disvm_relative_arrays_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_relative_native_bounds_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_relative_bounds.py" "$<TARGET_FILE:limbo-bootstrap>"
      "$<TARGET_FILE:disvm>" "${u}/module" "${mode}")
    set_tests_properties(disvm_relative_native_bounds_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Real source-to-Dis resized writes, including rune strings and one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_splice_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_splice.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_splice_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# A handled application error may resume, but policy budgets remain terminal.
if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_hosted_exception_boundary COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_exception_boundary.py" "$<TARGET_FILE:disvm>"
    "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
  set_tests_properties(disvm_hosted_exception_boundary PROPERTIES TIMEOUT 60
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

# Source scopes survive optimization and execute with both scheduler quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_leases_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_leases.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_leases_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()


# FIR ownership dataflow must agree with native recursive alias behavior.
if(DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_lease_flow_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/lease_flow.py" "${DVM_FIR_COMPILER}"
      "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_lease_flow_${mode} PROPERTIES TIMEOUT 30)
  endforeach()
endif()

# Source while/for-in must execute cyclic Dis in both VM modes and scheduler quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_source_loops_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/loops.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_source_loops_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_loop_control_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/loop_control.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_loop_control_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_counted_for_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/counted_for.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_counted_for_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()
