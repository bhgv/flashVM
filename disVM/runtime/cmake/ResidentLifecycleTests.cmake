# Copyright (c) 2026 bhgv
# EN: Ordered ResidentLifecycleTests registrations; include from the runtime source directory.
# RU: Регистрация ResidentLifecycleTests; подключать из каталога исходников runtime.

# EN: Whole-VM stale-reply ownership probe; no x64-only opcode instrumentation.
# RU: Проверка владельцев старого ответа в настоящей VM, без x64-specific hook.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_live_parent_late_reply COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_late_reply.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_live_parent_late_reply PROPERTIES TIMEOUT 180)
    if(CMAKE_C_FLAGS MATCHES "-fsanitize=thread")
        add_test(NAME disvm_live_parent_late_reply_race_control COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_late_reply.py" "$<TARGET_FILE:disvm>"
            "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --race-control)
        set_tests_properties(disvm_live_parent_late_reply_race_control PROPERTIES TIMEOUT 180)
    endif()
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_string_branches COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_string_branch.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_string_branch_tails COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_string_branch_tail.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_string_branches disvm_string_branch_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_real_branches COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_real_branch.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_real_branch_tails COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_real_branch_tail.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_real_branches disvm_real_branch_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_native_root_replacement COMMAND "${CMAKE_COMMAND}" -E env
        "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_root_replace.py")
    set_tests_properties(disvm_native_root_replacement PROPERTIES TIMEOUT 240)
endif()
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_managed_root_replacement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_root_replace.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_managed_root_replacement PROPERTIES TIMEOUT 240)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_shared_real_arithmetic COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_real_arithmetic.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_real_arithmetic_tails COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_real_arithmetic_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_shared_real_arithmetic disvm_real_arithmetic_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_guarded_dispatch_closure COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_guarded_dispatch.py" "${CMAKE_CURRENT_BINARY_DIR}/native")
    set_tests_properties(disvm_guarded_dispatch_closure PROPERTIES TIMEOUT 120)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_shared_real_conversion COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_real_conversion.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_real_conversion_tails COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_real_conversion_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_shared_real_conversion disvm_real_conversion_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_generic_heap_holds COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_heap_hold.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_generic_heap_holds PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_dedicated_spawn_portable COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_continuation.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --portable-overlap)
    set_tests_properties(disvm_dedicated_spawn_portable PROPERTIES TIMEOUT 150)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_record_new COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_record_new.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_record_new PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_module_continuation COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_continuation.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --dedicated-only)
    set_tests_properties(disvm_module_continuation PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_module_continuation_profiles COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_continuation.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_module_continuation_profiles PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_channel_guest_wait COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --dedicated-waits-only)
    add_test(NAME disvm_dedicated_channel_close COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --close-only)
    add_test(NAME disvm_dedicated_cancelwait COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --cancelwait-only)
    add_test(NAME disvm_dedicated_table_cancel COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-cancel-only)
    add_test(NAME disvm_dedicated_channel_buffer COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --dedicated-buffer-only)
    set_tests_properties(disvm_dedicated_channel_buffer PROPERTIES TIMEOUT 30)
    add_test(NAME disvm_dedicated_controller_wake COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --controller-wake-only)
    set_tests_properties(disvm_dedicated_controller_wake PROPERTIES TIMEOUT 30)
    add_test(NAME disvm_dedicated_table_fifo COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-fifo-only)
    add_test(NAME disvm_dedicated_table_kill_head COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-kill-head-only)
    add_test(NAME disvm_dedicated_table_cancel_head COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-cancel-head-only)
    set_tests_properties(disvm_dedicated_table_kill_head disvm_dedicated_table_cancel_head
        PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_dedicated_table_fifo PROPERTIES TIMEOUT 180)
    set_tests_properties(disvm_dedicated_table_cancel PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_dedicated_cancelwait PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_dedicated_channel_close PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_channel_guest_wait PROPERTIES TIMEOUT 90)
    add_test(NAME disvm_mapped_prepare COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_mapped_prepare.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_mapped_prepare PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_mapped_cons COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_mapped_cons.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_mapped_cons PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_mapped_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_mapped_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_mapped_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_raw_cons COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_raw_cons.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_raw_cons PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_raw_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_raw_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_raw_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_scalar_cons COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_scalar_cons.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_scalar_cons PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_scalar_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_scalar_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_scalar_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_value_prepare COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_value_prepare.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_value_prepare PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_pointer_transaction COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_pointer_transaction.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_pointer_transaction PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_guarded_pointer_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_pointer_tail.py" --guarded-copies "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_guarded_pointer_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_pointer_prepare COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_pointer_prepare.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_pointer_prepare PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_heap_commit COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_heap_commit.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_heap_commit PROPERTIES TIMEOUT 360)
endif()


if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(disvm_host_cancellations_test test_host_cancellations.c host_cancellations.c)
    target_link_libraries(disvm_host_cancellations_test PRIVATE pthread)
    add_test(NAME disvm_host_cancellations COMMAND disvm_host_cancellations_test)
    set_tests_properties(disvm_host_cancellations PROPERTIES TIMEOUT 10)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(disvm_owner_messages_test test_owner_messages.c owner_messages.c)
    target_link_libraries(disvm_owner_messages_test PRIVATE pthread)
    add_test(NAME disvm_owner_messages COMMAND disvm_owner_messages_test)
    set_tests_properties(disvm_owner_messages PROPERTIES TIMEOUT 15)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_resident_startup COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_startup.py" "$<TARGET_FILE:disvm>"
        "${CMAKE_CURRENT_BINARY_DIR}/managed-rc-overlap.dis")
    set_tests_properties(disvm_resident_startup PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_resident_alt COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_alt.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_resident_alt PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_resident_alt_mapped COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_alt.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --mapped)
    set_tests_properties(disvm_resident_alt_mapped PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_resident_processctl COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_processctl.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_parent_reply_exit COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_processctl.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --parent-exit)
    add_test(NAME disvm_channel_gc_snapshot COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_processctl.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --audit-query)
    set_tests_properties(disvm_resident_processctl disvm_parent_reply_exit disvm_channel_gc_snapshot PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_stream_kill COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_processctl.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --stream-kill)
    set_tests_properties(disvm_resident_stream_kill PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_receiver_kill COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_processctl.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --receiver-kill)
    set_tests_properties(disvm_resident_receiver_kill PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_receiver_cancel COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_processctl.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --receiver-cancel)
    set_tests_properties(disvm_resident_receiver_cancel PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_table_handoff COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_processctl.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-handoff)
    set_tests_properties(disvm_resident_table_handoff PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_table_retirement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_table_retirement.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_table_scope_kill COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_table_scope_kill.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_table_transfer COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_table_transfer.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_table_fifo_edit COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_table_fifo_edit.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_table_parent_exit COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_table_parent_exit.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_table_retirement disvm_table_scope_kill disvm_table_transfer disvm_table_fifo_edit disvm_table_parent_exit PROPERTIES TIMEOUT 180)
    add_executable(disvm_decimal_threads_test "${root}/tools/limbo-bootstrap/host/test_decimal_threads.c")
    target_link_libraries(disvm_decimal_threads_test PRIVATE limbo_bootstrap_support limbo_bootstrap_lib9 limbo_bootstrap_support)
    add_test(NAME disvm_decimal_threads COMMAND disvm_decimal_threads_test)
    set_tests_properties(disvm_decimal_threads PROPERTIES TIMEOUT 60)
    add_test(NAME disvm_resident_decimal COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_decimal.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_resident_decimal PROPERTIES TIMEOUT 180)
    foreach(unit format error)
        add_executable(disvm_${unit}_threads_test "${root}/tools/limbo-bootstrap/host/test_${unit}_threads.c")
        target_link_libraries(disvm_${unit}_threads_test PRIVATE limbo_bootstrap_support limbo_bootstrap_lib9 limbo_bootstrap_support)
        add_test(NAME disvm_${unit}_threads COMMAND disvm_${unit}_threads_test)
        set_tests_properties(disvm_${unit}_threads PROPERTIES TIMEOUT 60)
    endforeach()
    add_test(NAME disvm_resident_errors COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_errors.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_resident_errors PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_native_count COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_native_count.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_resident_native_count PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_process_roots COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_process_roots.py" "$<TARGET_FILE:disvm>")
    set_tests_properties(disvm_process_roots PROPERTIES TIMEOUT 60)
    add_test(NAME disvm_resident_root_probe COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_root_probe.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_resident_root_probe PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_jit_publication COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_jit_publish.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_jit_publication PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_string_view COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_string_view.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_string_view PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_mapped_accounting COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_mapped_accounting.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_mapped_accounting PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_gc_requests COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_gc_requests.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_gc_requests PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_module_registry COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_registry.py" "$<TARGET_FILE:disvm>")
    add_test(NAME disvm_load_kill COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_load_kill.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_load_kill PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_load COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_load.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_module_registry disvm_resident_load PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_tables COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_tables.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_resident_tables PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_native_kill COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_native_kill.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_native_kill PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_native_cancel COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_native_cancel.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_native_cancel PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_close_backlog COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_close_backlog.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_close_backlog PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_resident_files COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_resident_files.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_resident_files PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_file_gate_churn COMMAND "${CMAKE_COMMAND}" -E env
        "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_file_gate_churn.py")
    set_tests_properties(disvm_file_gate_churn PROPERTIES TIMEOUT 90)
endif()
