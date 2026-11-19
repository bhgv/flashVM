# Copyright (c) 2026 bhgv
# EN: Keep pure ABI/guard/generator checks without starting a reference VM.
# RU: Сохранить проверки ABI/guards/генератора без запуска reference VM.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_executable(disvm_styx_codec_test styx/test_codec.c)
    target_link_libraries(disvm_styx_codec_test PRIVATE limbo_bootstrap_lib9)
    add_test(NAME disvm_styx_c_codec COMMAND disvm_styx_codec_test)
    set_tests_properties(disvm_styx_c_codec PROPERTIES TIMEOUT 10 LABELS "host-unit")
    get_target_property(native_unit_includes disvm INCLUDE_DIRECTORIES)
    list(APPEND native_unit_includes "${DVM_FLASHVM_SOURCE_ROOT}/ports" "${DVM_FLASHVM_SOURCE_ROOT}/include")
    add_executable(disvm_sys_capture_test test_sys_capture.c sys_capture.c "${root}/disVM/host/refcount.c")
    add_dependencies(disvm_sys_capture_test disvm_runtime_sources)
    target_include_directories(disvm_sys_capture_test PRIVATE ${native_unit_includes})
    target_compile_definitions(disvm_sys_capture_test PRIVATE DVM_NATIVE_REGISTERS=1)
    add_executable(disvm_sys_owner_race_test test_sys_owner_race.c sys_capture.c "${root}/disVM/host/refcount.c")
    add_dependencies(disvm_sys_owner_race_test disvm_runtime_sources)
    target_include_directories(disvm_sys_owner_race_test PRIVATE ${native_unit_includes})
    target_compile_definitions(disvm_sys_owner_race_test PRIVATE DVM_NATIVE_REGISTERS=1)
    target_link_libraries(disvm_sys_owner_race_test PRIVATE Threads::Threads)
    add_test(NAME disvm_sys_owner_race COMMAND disvm_sys_owner_race_test)
    set_tests_properties(disvm_sys_owner_race PROPERTIES TIMEOUT 30 LABELS "host-unit")
    foreach(capture_case RANGE 0 18)
        add_test(NAME disvm_sys_capture_${capture_case} COMMAND disvm_sys_capture_test ${capture_case})
        set_tests_properties(disvm_sys_capture_${capture_case} PROPERTIES TIMEOUT 10 LABELS "host-unit")
    endforeach()
    add_test(NAME disvm_runtime_platform_selection COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_platform.py" "${CMAKE_COMMAND}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RuntimePlatform.cmake")
    set_tests_properties(disvm_runtime_platform_selection PROPERTIES TIMEOUT 30 LABELS "host-unit")
    add_executable(disvm_fp_records_test test_jit_fp_records.c jit_fault/fpsimd_records.c
        jit_fault/fp_view.c jit_fault/fp_context.c jit_fault/context.c)
    target_include_directories(disvm_fp_records_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME disvm_native_fp_records COMMAND disvm_fp_records_test)
    set_tests_properties(disvm_native_fp_records PROPERTIES TIMEOUT 10 LABELS "host-unit")
    add_executable(disvm_vfs_handle_test vfs/test_handle.c vfs/handle.c vfs/io_result.c)
    target_include_directories(disvm_vfs_handle_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_vfs_handle_test PRIVATE disvm_flashvm_host vfs_walk)
    add_test(NAME disvm_vfs_handle COMMAND disvm_vfs_handle_test)
    set_tests_properties(disvm_vfs_handle PROPERTIES TIMEOUT 10 LABELS "host-unit")
    add_executable(disvm_sysevents_test vfs/test_sysevents.c vfs/test_notice.c vfs/test_sysevents_queue.c
        vfs/test_sysevents_race.c vfs/test_sysevents_subscriptions.c vfs/test_sysevents_device.c vfs/sysevents.c vfs/sysevents_watch.c
        vfs/sysevents_stream.c vfs/sysevents_device.c owner_messages.c vfs/path.c vfs/group.c vfs/mutations.c
        vfs/layer_bind.c vfs/cancel.c vfs/union_io.c vfs/directory.c vfs/directory_names.c vfs/auto_access.c vfs/resolution.c vfs/resource.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c
        vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c vfs/handle.c vfs/io_result.c vfs/anchor.c vfs/resource_groups.c)
    target_include_directories(disvm_sysevents_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_sysevents_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    set_property(TARGET disvm_sysevents_test APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=calloc -Wl,--wrap=dvm_provider_create")
    add_test(NAME disvm_sysevents COMMAND disvm_sysevents_test)
    set_tests_properties(disvm_sysevents PROPERTIES TIMEOUT 20 LABELS "host-unit")
    add_test(NAME disvm_sysevents_device_control COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vfs/test_sysevents_device_control.py" "$<TARGET_FILE:disvm_sysevents_test>")
    set_tests_properties(disvm_sysevents_device_control PROPERTIES TIMEOUT 90 LABELS "host-unit")
    add_executable(disvm_stack_test vfs/test_stack.c vfs/stack.c vfs/stack_target.c vfs/path.c vfs/group.c vfs/mutations.c
        vfs/layer_bind.c vfs/cancel.c vfs/union_io.c vfs/directory.c vfs/directory_names.c vfs/auto_access.c vfs/resolution.c vfs/resource.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c
        vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c vfs/handle.c vfs/io_result.c vfs/anchor.c vfs/resource_groups.c)
    target_include_directories(disvm_stack_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_stack_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    set_property(TARGET disvm_stack_test APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=calloc")
    add_test(NAME disvm_stack COMMAND disvm_stack_test)
    set_tests_properties(disvm_stack PROPERTIES TIMEOUT 20 LABELS "host-unit")
    add_executable(disvm_eventfs_test vfs/test_eventfs.c vfs/eventfs.c vfs/eventfile.c
        vfs/handle.c vfs/io_result.c vfs/cancel.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c)
    target_include_directories(disvm_eventfs_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_eventfs_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    add_test(NAME disvm_eventfs COMMAND disvm_eventfs_test)
    set_tests_properties(disvm_eventfs PROPERTIES TIMEOUT 20 LABELS "host-unit")
    add_executable(disvm_changes_test vfs/test_changes.c vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c)
    target_include_directories(disvm_changes_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_changes_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    add_test(NAME disvm_changes COMMAND disvm_changes_test)
    set_tests_properties(disvm_changes PROPERTIES TIMEOUT 20 LABELS "host-unit")
    add_executable(disvm_io_result_test vfs/test_io_result.c vfs/handle.c vfs/io_result.c vfs/cancel.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c)
    target_include_directories(disvm_io_result_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_io_result_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    add_test(NAME disvm_io_result COMMAND disvm_io_result_test)
    set_tests_properties(disvm_io_result PROPERTIES TIMEOUT 10 LABELS "host-unit")
    add_executable(disvm_provider_mount_test vfs/test_provider_mount.c vfs/path.c vfs/group.c vfs/mutations.c vfs/layer_bind.c vfs/cancel.c vfs/union_io.c vfs/directory.c vfs/directory_names.c vfs/auto_access.c
        vfs/resolution.c vfs/resource.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c vfs/handle.c vfs/io_result.c vfs/anchor.c vfs/resource_groups.c)
    target_include_directories(disvm_provider_mount_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_provider_mount_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    add_test(NAME disvm_provider_mount COMMAND disvm_provider_mount_test)
    set_tests_properties(disvm_provider_mount PROPERTIES TIMEOUT 20 LABELS "host-unit")
    add_executable(disvm_layer_order_test vfs/test_layer_order.c vfs/path.c vfs/group.c vfs/mutations.c
        vfs/layer_bind.c vfs/cancel.c vfs/union_io.c vfs/directory.c vfs/directory_names.c vfs/auto_access.c vfs/resolution.c vfs/resource.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c vfs/handle.c vfs/io_result.c
        vfs/anchor.c vfs/resource_groups.c)
    target_include_directories(disvm_layer_order_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_layer_order_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    add_test(NAME disvm_layer_order COMMAND disvm_layer_order_test)
    set_tests_properties(disvm_layer_order PROPERTIES TIMEOUT 20 LABELS "host-unit")
    add_executable(disvm_auto_access_test vfs/test_auto_access.c vfs/path.c vfs/group.c vfs/mutations.c
        vfs/layer_bind.c vfs/cancel.c vfs/union_io.c vfs/directory.c vfs/directory_names.c vfs/auto_access.c vfs/resolution.c vfs/resource.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c
        vfs/handle.c vfs/io_result.c vfs/anchor.c vfs/resource_groups.c)
    target_include_directories(disvm_auto_access_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_auto_access_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    add_test(NAME disvm_auto_access COMMAND disvm_auto_access_test)
    set_tests_properties(disvm_auto_access PROPERTIES TIMEOUT 20 LABELS "host-unit")
    add_executable(disvm_namespace_test vfs/test_namespace.c vfs/path.c vfs/group.c vfs/mutations.c vfs/layer_bind.c vfs/cancel.c vfs/union_io.c vfs/directory.c vfs/directory_names.c vfs/auto_access.c
        vfs/observation.c vfs/resolution.c vfs/provider.c vfs/provider_capture.c vfs/provider_walk.c vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c vfs/handle.c vfs/io_result.c vfs/resource.c vfs/anchor.c vfs/resource_groups.c)
    target_include_directories(disvm_namespace_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_namespace_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    add_test(NAME disvm_namespace_concurrent COMMAND disvm_namespace_test)
    add_test(NAME disvm_anchor_port COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_anchor_port.py" "$<TARGET_FILE:disvm_anchor_port_test>")
    set_tests_properties(disvm_anchor_port PROPERTIES TIMEOUT 50 LABELS "host-unit")
    set_tests_properties(disvm_namespace_concurrent PROPERTIES TIMEOUT 30 LABELS "host-unit")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_native_generation_idempotent COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_generation.py" "$<TARGET_FILE:limbo-bootstrap>"
        "${u}/libinterp" "${view}")
    set_tests_properties(disvm_native_generation_idempotent PROPERTIES TIMEOUT 30 LABELS "host-unit"
        ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

# EN/RU: Build the actual target ELF for explicit QEMU acceptance too.
# Собирать настоящий target ELF также для явной QEMU-приёмки.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    get_target_property(anchor_includes disvm INCLUDE_DIRECTORIES)
    list(APPEND anchor_includes "${DVM_FLASHVM_SOURCE_ROOT}/ports" "${DVM_FLASHVM_SOURCE_ROOT}/include")
    add_executable(disvm_anchor_port_test vfs/test_anchor_port.c)
    target_include_directories(disvm_anchor_port_test PRIVATE ${anchor_includes})
    target_link_libraries(disvm_anchor_port_test PRIVATE disvm_flashvm_host vfs_walk Threads::Threads)
    target_link_options(disvm_anchor_port_test PRIVATE "-Wl,--wrap=fdopen")
endif()

add_test(NAME disvm_system_identity COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_system_identity.py")
set_tests_properties(disvm_system_identity PROPERTIES TIMEOUT 30 LABELS "host-unit")

# EN: Check the portable guest and relative imports through the shipped launcher.
# RU: Проверять переносимого гостя и относительные импорты через штатный launcher.
if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_EDITOR AND UNIX)
    add_test(NAME disvm_desktop_installed COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_install.py"
        "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/run-desktop-ide.sh")
    set_tests_properties(disvm_desktop_installed PROPERTIES TIMEOUT 200 LABELS "native;editor")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME disvm_editor_idle_cpu COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_editor_idle.py" "$<TARGET_FILE:disvm-editor>"
            "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
            "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf")
        set_tests_properties(disvm_editor_idle_cpu PROPERTIES TIMEOUT 90 LABELS "native;editor")
    endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/CompilerTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NativeModuleTests.cmake")
