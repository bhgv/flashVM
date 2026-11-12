# Copyright (c) 2026 bhgv
# EN: Ordered ParallelExecutionTests registrations; include from the runtime source directory.
# RU: Регистрация ParallelExecutionTests; подключать из каталога исходников runtime.

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_dense_selection_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_dense_selection.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_vector_dense_selection_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# Latest-only geometry scheduling is distinct from ordered native input capture.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis"
    "${root}/ide/dis/tests/ideinklatest_test.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/tests/ideinklatest_test.b" ${module_headers} VERBATIM)
add_custom_target(disvm_inklatest_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_inklatest_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis" ${mode} 90)
    set_tests_properties(disvm_inklatest_${mode} PROPERTIES TIMEOUT 100)
  endforeach()
endif()

# Generic copied point modifiers run without a Dis or FlashVM heap reference.
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_vector_local_test test_vector_local.c vector_local.c vector_transform.c)
  target_link_libraries(disvm_vector_local_test PRIVATE m)
  add_test(NAME disvm_vector_local COMMAND disvm_vector_local_test)
endif()

# Whole managed applications, not synthetic register-only callbacks, on a Dis OS owner.
if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_EDITOR)
  add_test(NAME disvm_threaded_ui_owners COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_threaded_editor.py" "$<TARGET_FILE:disvm-editor>"
    "${CMAKE_CURRENT_BINARY_DIR}/idecore-test.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
  set_tests_properties(disvm_threaded_ui_owners PROPERTIES TIMEOUT 120)
endif()

# Runnable managed computation example; no synthetic replacement VM or halt exit.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/threaded-owner.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/threaded-owner.dis"
    "${root}/disVM/examples/threaded-owner/main.b"
  DEPENDS limbo-bootstrap "${root}/disVM/examples/threaded-owner/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_threaded_owner_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/threaded-owner.dis")

# Real managed cyclic garbage, preserved roots, native collector joins/errors.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
   -o "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis"
   "${root}/disVM/examples/gc-worker/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-worker/main.b" VERBATIM)
add_custom_target(disvm_gc_worker_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis")
add_test(NAME disvm_native_cycle_gc COMMAND "${PYTHON_EXECUTABLE}"
 "${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_gc.py" "$<TARGET_FILE:disvm>"
 "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis")
set_tests_properties(disvm_native_cycle_gc PROPERTIES TIMEOUT 180 PROCESSORS 4)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_dedicated_cycle_gc COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_gc.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis" --dedicated-only)
 set_tests_properties(disvm_dedicated_cycle_gc PROPERTIES TIMEOUT 180 PROCESSORS 4)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_idle_cycle_gc COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_gc.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis" --idle-cycles)
 set_tests_properties(disvm_idle_cycle_gc PROPERTIES TIMEOUT 130 PROCESSORS 4)
 set(record_cycle_overlap_module "${CMAKE_CURRENT_BINARY_DIR}/record-cycle-overlap.dis")
 add_custom_command(OUTPUT "${record_cycle_overlap_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${record_cycle_overlap_module}" "${CMAKE_CURRENT_SOURCE_DIR}/record_cycle_overlap.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/record_cycle_overlap.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_record_cycle_overlap_module ALL DEPENDS "${record_cycle_overlap_module}")
 add_test(NAME disvm_record_cycle_overlap COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${record_cycle_overlap_module}")
 set_tests_properties(disvm_record_cycle_overlap PROPERTIES TIMEOUT 330)
 set(container_cycle_overlap_module "${CMAKE_CURRENT_BINARY_DIR}/container-cycle-overlap.dis")
 add_custom_command(OUTPUT "${container_cycle_overlap_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${container_cycle_overlap_module}" "${CMAKE_CURRENT_SOURCE_DIR}/container_cycle_overlap.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/container_cycle_overlap.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_container_cycle_overlap_module ALL DEPENDS "${container_cycle_overlap_module}")
 add_test(NAME disvm_container_cycle_overlap COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${container_cycle_overlap_module}" --native-spawn --require-confirmation)
 set_tests_properties(disvm_container_cycle_overlap PROPERTIES TIMEOUT 330)
 set(large_record_cycle_module "${CMAKE_CURRENT_BINARY_DIR}/large-record-cycle.dis")
 add_custom_command(OUTPUT "${large_record_cycle_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${large_record_cycle_module}" "${CMAKE_CURRENT_SOURCE_DIR}/large_record_cycle.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/large_record_cycle.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_large_record_cycle_module ALL DEPENDS "${large_record_cycle_module}")
 add_test(NAME disvm_large_record_cycle COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${large_record_cycle_module}" --native-spawn --large-cycle)
 set_tests_properties(disvm_large_record_cycle PROPERTIES TIMEOUT 330)
 set(live_branch_charge_module "${CMAKE_CURRENT_BINARY_DIR}/live-branch-charge.dis")
 add_custom_command(OUTPUT "${live_branch_charge_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${live_branch_charge_module}" "${CMAKE_CURRENT_SOURCE_DIR}/gc/fast/live_branch_charge.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/gc/fast/live_branch_charge.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_live_branch_charge_module ALL DEPENDS "${live_branch_charge_module}")
 add_test(NAME disvm_live_branch_charge COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${live_branch_charge_module}" --native-spawn --live-branch-charge)
 set_tests_properties(disvm_live_branch_charge PROPERTIES TIMEOUT 330)




endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_parallel_gc_graph COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/inferno_gc.py"
  --workers --flashvm-root "${root}/flashVM")
 set_tests_properties(disvm_parallel_gc_graph PROPERTIES TIMEOUT 60)
endif()
if(TARGET disvm-editor)
 add_test(NAME disvm_editor_native_cycle_gc COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_gc.py" "$<TARGET_FILE:disvm-editor>"
  "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis")
 set_tests_properties(disvm_editor_native_cycle_gc PROPERTIES TIMEOUT 180)
endif()

# Audited native callback: real managed roots remain alive outside the heap lease.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis"
  "${root}/disVM/examples/gc-native-wait/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-wait/main.b" VERBATIM)
add_custom_target(disvm_native_wait_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_wait COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_wait.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
 set_tests_properties(disvm_native_wait PROPERTIES TIMEOUT 90)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_wait COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_wait.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
  set_tests_properties(disvm_editor_native_wait PROPERTIES TIMEOUT 90)
 endif()
endif()

# Native read/readn staging, external roots, real delayed OS pipes and file handles.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis"
  "${root}/disVM/examples/gc-native-read/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-read/main.b" VERBATIM)
add_custom_target(disvm_native_read_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_read COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_read.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis")
 set_tests_properties(disvm_native_read PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_read COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_read.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis")
  set_tests_properties(disvm_editor_native_read PROPERTIES TIMEOUT 120)
 endif()
endif()

# Immutable managed-to-native output snapshots with actual pipe backpressure.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis"
  "${root}/disVM/examples/gc-native-write/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-write/main.b" VERBATIM)
add_custom_target(disvm_native_write_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_write COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_write.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis")
 set_tests_properties(disvm_native_write PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_write COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_write.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis")
  set_tests_properties(disvm_editor_native_write PROPERTIES TIMEOUT 120)
 endif()
endif()

# Formatted output releases with saved variadic roots and actual pipe backpressure.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis"
  "${root}/disVM/examples/gc-native-format/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-format/main.b" VERBATIM)
add_custom_target(disvm_native_format_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_format COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_format.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis")
 set_tests_properties(disvm_native_format PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_format COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_format.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis")
  set_tests_properties(disvm_editor_native_format PROPERTIES TIMEOUT 120)
 endif()
endif()


# Actual native owners share retained instruction operands through FIFO heap turns.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_parallel_gc_admission COMMAND "${PYTHON_EXECUTABLE}"
  "${root}/disVM/tests/native_serial.py" --flashvm-root "${root}/flashVM")
 set_tests_properties(disvm_parallel_gc_admission PROPERTIES TIMEOUT 80 ENVIRONMENT "CC=${CMAKE_C_COMPILER}")
endif()


# Trusted local native calls exercise terminal cleanup, not shared file-call ABI.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(call_gate native_local_calls native_local_calls_control)
  set(call_control)
  if(call_gate STREQUAL "native_local_calls_control")
   set(call_control --control)
  endif()
  add_test(NAME disvm_${call_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}"
   "CPATH=${root}/disVM/host/include:${u}/Linux/arm/include"
   "NATIVE64_ARCH=${runtime_backend}"
   "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
   "${PYTHON_EXECUTABLE}" "${u}/utils/tests/native64-local-calls.py" ${call_control})
  set_tests_properties(disvm_${call_gate} PROPERTIES TIMEOUT 120)
 endforeach()
endif()


# Per-context native-call state and registration sealing, with active controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_owner_leases COMMAND "${PYTHON_EXECUTABLE}"
  "${root}/disVM/tests/native_owner.py" --flashvm-root "${root}/flashVM")
 set_tests_properties(disvm_native_owner_leases PROPERTIES TIMEOUT 100 ENVIRONMENT "CC=${CMAKE_C_COMPILER}")
endif()


# Trusted caught-call recovery: actual emu/os handlers and whole-REG borrowers.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(handler_gate native_local_handlers native_local_handlers_pending native_local_handlers_registers)
  set(handler_control)
  if(handler_gate STREQUAL "native_local_handlers_pending")
   set(handler_control --control pending)
  elseif(handler_gate STREQUAL "native_local_handlers_registers")
   set(handler_control --control registers)
  endif()
  add_test(NAME disvm_${handler_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}"
   "CPATH=${root}/disVM/host/include:${u}/Linux/arm/include"
   "NATIVE64_ARCH=${runtime_backend}"
   "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
   "${PYTHON_EXECUTABLE}" "${u}/utils/tests/native64-local-handlers.py" ${handler_control})
  set_tests_properties(disvm_${handler_gate} PROPERTIES TIMEOUT 180)
 endforeach()
endif()


# Actual additional native owners inspect the production queue during audited waits.
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_queue_probe COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_queue_probe.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
 set_tests_properties(disvm_native_queue_probe PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_queue_probe COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_queue_probe.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
  set_tests_properties(disvm_editor_queue_probe PROPERTIES TIMEOUT 120)
 endif()
endif()


# Independent FIFO oracle and semantic native-owner admission controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_runqueue_ownership COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_runqueue.py")
 set_tests_properties(disvm_runqueue_ownership PROPERTIES TIMEOUT 60 ENVIRONMENT "CC=${CMAKE_C_COMPILER}")
endif()


# Fresh exception-message allocation must observe the recovered live caller roots.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(fresh_gate native_fresh_handlers native_fresh_handlers_publication native_fresh_handlers_map)
  set(fresh_control)
  if(fresh_gate STREQUAL "native_fresh_handlers_publication")
   set(fresh_control --control publication)
  elseif(fresh_gate STREQUAL "native_fresh_handlers_map")
   set(fresh_control --control map)
  endif()
  add_test(NAME disvm_${fresh_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}"
   "CPATH=${root}/disVM/host/include:${u}/Linux/arm/include"
   "NATIVE64_ARCH=${runtime_backend}"
   "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
   "${PYTHON_EXECUTABLE}" "${u}/utils/tests/native64-fresh-handlers.py" ${fresh_control})
  set_tests_properties(disvm_${fresh_gate} PROPERTIES TIMEOUT 240)
 endforeach()
endif()


# Real extended-up guards must remain owner-local outside serialized heap turns.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(owner_gate native_execution_owners native_execution_shared_control)
  set(owner_control)
  if(owner_gate STREQUAL "native_execution_shared_control")
   set(owner_control --control shared-execution)
  endif()
  add_test(NAME disvm_${owner_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}" "CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all"
   "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/native_registers.py"
   --flashvm-root "${DVM_FLASHVM_SOURCE_ROOT}" ${owner_control})
  set_tests_properties(disvm_${owner_gate} PROPERTIES TIMEOUT 60)
 endforeach()
endif()


# Private proposal checkers validate trusted metadata/IR only, never loader admission.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(local_family catalog flow state)
  if(local_family STREQUAL "catalog")
   set(local_controls normal entries tail)
  elseif(local_family STREQUAL "flow")
   set(local_controls normal branch fallthrough calls cycle depth)
  else()
   set(local_controls normal arguments binding reuse kind boolean return retire join
    canonical preparation address identity result)
  endif()
  foreach(local_control ${local_controls})
   set(local_arguments)
   if(NOT local_control STREQUAL "normal")
    set(local_arguments --control ${local_control})
   endif()
   set(local_gate disvm_dis64_private_${local_family}_${local_control})
   add_test(NAME ${local_gate} COMMAND "${CMAKE_COMMAND}" -E env
    "CC=${CMAKE_C_COMPILER}" "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
    "${PYTHON_EXECUTABLE}" "${u}/utils/tests/dis64-local-${local_family}.py" ${local_arguments})
   set_tests_properties(${local_gate} PROPERTIES TIMEOUT 180 LABELS "dis64-private")
  endforeach()
 endforeach()
endif()


# Real two-executor examples and native backpressure; no simulated dispatcher.
foreach(pool_example main rc-overlap backpressure backpressure-eight shared-file file-overlap metadata-overlap elastic elastic-stop elastic-park ui-rpc)
 if(pool_example STREQUAL "main")
  set(pool_image managed-workers)
 elseif(pool_example STREQUAL "rc-overlap")
  set(pool_image managed-rc-overlap)
 elseif(pool_example STREQUAL "backpressure")
  set(pool_image managed-backpressure)
 elseif(pool_example STREQUAL "backpressure-eight")
  set(pool_image managed-backpressure-eight)
 elseif(pool_example STREQUAL "elastic-park")
  set(pool_image managed-elastic-park)
 elseif(pool_example STREQUAL "elastic-stop")
  set(pool_image managed-elastic-stop)
 elseif(pool_example STREQUAL "elastic")
  set(pool_image managed-elastic)
 elseif(pool_example STREQUAL "metadata-overlap")
  set(pool_image managed-metadata-overlap)
 elseif(pool_example STREQUAL "file-overlap")
  set(pool_image managed-file-overlap)
 elseif(pool_example STREQUAL "shared-file")
  set(pool_image managed-shared-file)
 else()
  set(pool_image managed-ui-rpc)
 endif()
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${pool_image}.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module" -I "${root}/ide/dis/bridge"
   -o "${CMAKE_CURRENT_BINARY_DIR}/${pool_image}.dis"
   "${root}/disVM/examples/managed-workers/${pool_example}.b"
  DEPENDS limbo-bootstrap "${root}/disVM/examples/managed-workers/${pool_example}.b"
   "${root}/ide/dis/bridge/idehost.m" VERBATIM)
 list(APPEND pool_images "${CMAKE_CURRENT_BINARY_DIR}/${pool_image}.dis")
endforeach()
add_custom_target(disvm_managed_worker_modules ALL DEPENDS ${pool_images})
# Removed shared executor CLI must fail before VM or helper startup.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(pool_runtime disvm disvm-editor)
  if(TARGET ${pool_runtime})
   add_test(NAME ${pool_runtime}_removed_pool_rejected COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_removed_pool.py" "$<TARGET_FILE:${pool_runtime}>"
    "${CMAKE_CURRENT_BINARY_DIR}/managed-workers.dis")
   set_tests_properties(${pool_runtime}_removed_pool_rejected PROPERTIES TIMEOUT 30)
  endif()
 endforeach()
endif()


# Per-file FIFO uses genuine native contexts and active private removal controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_file_fifo COMMAND "${CMAKE_COMMAND}" -E env "CC=${CMAKE_C_COMPILER}"
  "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_file_gate.py")
 set_tests_properties(disvm_native_file_fifo PROPERTIES TIMEOUT 120)
endif()
