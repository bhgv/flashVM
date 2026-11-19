# Copyright (c) 2026 bhgv
# EN: Native publication/composition acceptance, separate from wire Styx/remote authentication.
# RU: Native publication/composition acceptance, отдельно от wire Styx/remote authentication.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_executable(disvm_export_test vfs/test_export.c vfs/test_export_listing.c vfs/test_export_race.c vfs/view.c vfs/view_capture.c vfs/view_launch.c vfs/launch/address_guard.c vfs/view_io.c vfs/view_list.c
        vfs/export_native.c vfs/publication.c vfs/stack.c vfs/stack_target.c vfs/path.c vfs/group.c vfs/mutations.c
        vfs/layer_bind.c vfs/cancel.c vfs/union_io.c vfs/directory.c vfs/directory_names.c vfs/auto_access.c vfs/resolution.c vfs/resource.c vfs/provider.c vfs/provider_capture.c vfs/provider_launch.c
        vfs/changes.c vfs/notice.c vfs/changes_refresh.c vfs/changes_ack.c vfs/handle.c vfs/io_result.c vfs/anchor.c vfs/resource_groups.c)
    target_include_directories(disvm_export_test PRIVATE ${native_unit_includes})
    target_link_libraries(disvm_export_test PRIVATE disvm_flashvm_host Threads::Threads)
    add_test(NAME disvm_export_tree COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_export_tree.py" "$<TARGET_FILE:disvm_export_test>")
    set_tests_properties(disvm_export_tree PROPERTIES TIMEOUT 40 LABELS "host-unit")
endif()
