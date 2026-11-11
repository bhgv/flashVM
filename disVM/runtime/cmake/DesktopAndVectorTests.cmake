# Copyright (c) 2026 bhgv
# EN: Ordered DesktopAndVectorTests registrations; include from the runtime source directory.
# RU: Регистрация DesktopAndVectorTests; подключать из каталога исходников runtime.

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
    -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis" "${root}/ide/dis/examples/desktop-pointer/main.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-pointer/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_pointer_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_pointer_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis")
    set_tests_properties(disvm_desktop_pointer_tutorial_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop pointer tutorial:")
  endforeach()
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
    -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis" "${root}/ide/dis/examples/desktop-clipboard/main.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-clipboard/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_clipboard_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_clipboard_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis")
    set_tests_properties(disvm_desktop_clipboard_tutorial_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop clipboard tutorial:")
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_clipboard_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_clipboard.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_clipboard_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis"
      "${root}/ide/dis/tests/deskclip_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/deskclip_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_deskclip_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis")
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_clipboard_guards_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 --max-instructions 50000000 "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis")
    set_tests_properties(disvm_desktop_clipboard_guards_${mode} PROPERTIES TIMEOUT 90
      PASS_REGULAR_EXPRESSION "PASS desktop clipboard guards:")
  endforeach()
endif()

# Exercise normalized gestures against actual Dis policy and MovieClip pixels.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_retained_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_retained.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_retained_${mode} PROPERTIES TIMEOUT 90)
    add_test(NAME disvm_desktop_pool_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_pool.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_pool_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_rails_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_rails.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_rails_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_mdi_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_mdi.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_mdi_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_debugger_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_debugger.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_debugger_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_vector_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_vector.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_vector_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_vector_mouse_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_vector.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode} mouse)
    set_tests_properties(disvm_desktop_vector_mouse_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_syntax_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_syntax.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_syntax_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_focus_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_focus.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_desktop_focus_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_workspace_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_workspace.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_desktop_workspace_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_glass_motion_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_glass_motion.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_desktop_glass_motion_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Run raw/optimized integer identities in the actual matched Dis engines.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_integer_identities_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_identities.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_identities_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Cross-target checked shift admission shares the production FIR libraries.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_swf_int_shifts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/swf_int_shifts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_swf_int_shifts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# The same signed int32 division/remainder results must reach both actual engines.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_swf_int_divmod_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/swf_int_divmod.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_swf_int_divmod_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_total_arithmetic_dce_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/total_arithmetic_dce.py" "${DVM_FIR_COMPILER}"
      "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_total_arithmetic_dce_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Build the same structural codec tools beside the separate NC64 runtime.
add_subdirectory("${root}/disVM/envelope" envelope)

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_integer_pairs_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_pairs.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_pairs_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interp)
    add_test(NAME disvm_fir_integer_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_facts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interp)
    add_test(NAME disvm_fir_integer_shift_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_shift_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_shift_facts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Frame-free engine scheduling and SDL text-burst boundaries use real owner APIs.
if(DVM_RUNTIME_TESTS AND TARGET flashvm)
  foreach(name async timers timers_budget timers_next_delay timers_versions_conflicts stage_update)
    add_executable(disvm_flashvm_${name}_test "${DVM_FLASHVM_SOURCE_ROOT}/tests/${name}.cpp")
    target_link_libraries(disvm_flashvm_${name}_test PRIVATE flashvm)
    add_test(NAME disvm_flashvm_${name} COMMAND disvm_flashvm_${name}_test)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interp)
    add_test(NAME disvm_fir_integer_cast_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_cast_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_cast_facts_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_integer_power_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_power_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_power_facts_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_integer_known_results_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_known_results.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_known_results_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Real owning-compiler regression: preserve old array values through initializer
# evaluation and publish the completed temporary only afterwards. Both optimizer
# selections execute on the matching native-cell VM in interpreter/JIT modes.
if(DVM_RUNTIME_TESTS)
  foreach(selection plain optimized)
    set(alias_flags)
    if(selection STREQUAL "optimized")
      set(alias_flags -O)
    endif()
    set(alias_binary "${CMAKE_CURRENT_BINARY_DIR}/array-alias-${selection}.dis")
    add_custom_command(OUTPUT "${alias_binary}"
      COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${alias_flags} ${includes} -o "${alias_binary}"
        "${u}/utils/tests/array-initializer-alias.b"
      DEPENDS limbo-bootstrap "${u}/utils/tests/array-initializer-alias.b" VERBATIM)
    add_custom_target(disvm_array_alias_${selection} ALL DEPENDS "${alias_binary}")
    foreach(mode jit interpreter)
      add_test(NAME disvm_array_initializer_${selection}_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>" "${alias_binary}" ${mode})
      set_tests_properties(disvm_array_initializer_${selection}_${mode} PROPERTIES TIMEOUT 60)
    endforeach()
  endforeach()
endif()

# Every consumer of desktop_view links the same generic primitives and AS resources.
if(TARGET disvm_chrome_headers)
  foreach(consumer disvm-editor disvm_desktop_colors_test disvm_mdi_capture_test disvm_desktop_vector_test)
    if(TARGET ${consumer})
      add_dependencies(${consumer} disvm_chrome_headers)
    endif()
  endforeach()
endif()

# Run the new proof through ordinary interpreter/native JIT execution, not a model.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_sparse_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_sparse_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_sparse_facts_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_fir_division_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_division_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_division_facts_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# Reuse the real editor configuration with a fixture entry replacing only main.
# The copied native buffers are consumed by the production Limbo geometry worker.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  get_target_property(ink_worker_sources disvm-editor SOURCES)
  list(REMOVE_ITEM ink_worker_sources main.c)
  add_executable(disvm_native_ink_runner test_ink_runner.c ${ink_worker_sources})
  if(DVM_EDITOR_SDL)
    target_sources(disvm_native_ink_runner PRIVATE test_live_ink.c)
  endif()
  foreach(property INCLUDE_DIRECTORIES COMPILE_DEFINITIONS COMPILE_OPTIONS LINK_OPTIONS)
    get_target_property(ink_worker_property disvm-editor ${property})
    if(ink_worker_property)
      set_property(TARGET disvm_native_ink_runner PROPERTY ${property} "${ink_worker_property}")
    endif()
  endforeach()
  get_target_property(ink_worker_libraries disvm-editor LINK_LIBRARIES)
  target_link_libraries(disvm_native_ink_runner PRIVATE ${ink_worker_libraries})
  add_dependencies(disvm_native_ink_runner disvm_runtime_sources disvm_view_headers disvm_chrome_headers)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
      -o "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis" "${root}/ide/dis/tests/native_ink_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/native_ink_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_native_ink_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis")
  foreach(mode jit interpreter)
    add_test(NAME disvm_native_ink_worker_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_native_ink.py" "$<TARGET_FILE:disvm_native_ink_runner>"
      "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_native_ink_worker_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(TARGET disvm_native_ink_runner AND DVM_EDITOR_SDL)
  add_test(NAME disvm_native_ink_sdl COMMAND disvm_native_ink_runner --sdl-fixture "${DVM_EDITOR_TEST_FONT}")
  set_tests_properties(disvm_native_ink_sdl PROPERTIES TIMEOUT 60
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_maximize_native_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_maximize_native.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_maximize_native_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  add_executable(disvm_ink_primitives_test test_ink_primitives.c editor_ink.c editor_stamp.c)
  target_link_libraries(disvm_ink_primitives_test PRIVATE flashvm m)
  add_test(NAME disvm_native_ink_primitives COMMAND disvm_ink_primitives_test)
endif()

# Scroll batching preserves exact legacy distance and returned viewport pixels.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_scroll_batch_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_scroll_batch.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_scroll_batch_${mode} PROPERTIES TIMEOUT 240)
  endforeach()
endif()

# Tool-state transitions must not reserialize/rasterize already committed vectors.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_control_cache_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_controls_cache.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_vector_control_cache_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Two real document owners must make progress through separate geometry queues.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_bulk_ports_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/bulkports-test.dis" "${mode}")
    set_tests_properties(disvm_bulk_ports_${mode} PROPERTIES TIMEOUT 60
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME disvm_bulk_geometry_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_bulk_transport.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_bulk_geometry_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# Ordinary full-frame commands must preserve open strokes and compound fill rules.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  add_executable(disvm_full_strokes_test test_full_strokes.c editor_raster.c editor_stamp.c)
  target_link_libraries(disvm_full_strokes_test PRIVATE flashvm m)
  add_test(NAME disvm_full_strokes COMMAND disvm_full_strokes_test)
endif()

# Independent committed Line and Pen colors must survive real authoring and SVG save.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_tool_modes_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_tool_modes.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_vector_tool_modes_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# The finite overview is a real fixed MovieClip layer with live source mirrors.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_atlas_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_atlas.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_atlas_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_editing_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_editing.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_vector_editing_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_viewport_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_viewport.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_vector_viewport_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis"
    "${root}/ide/dis/tests/millisec_test.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/tests/millisec_test.b" VERBATIM)
add_custom_target(disvm_millisec_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis")
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_clock_port_test test_clock_port.c)
  target_link_libraries(disvm_clock_port_test PRIVATE disvm_flashvm_host)
  add_test(NAME disvm_monotonic_clock_port COMMAND disvm_clock_port_test)
  foreach(mode jit interpreter)
    add_test(NAME disvm_millisec_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis" ${mode})
  endforeach()
endif()
