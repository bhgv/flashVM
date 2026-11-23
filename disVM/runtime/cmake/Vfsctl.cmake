# Copyright (c) 2026 bhgv
# EN: Generate standalone scalar VFS builtin and compile its native implementation.
# RU: Генерировать standalone scalar VFS builtin и компилировать native implementation.
add_custom_command(OUTPUT "${view}/vfsnative_decl.h" "${view}/vfsnative_table.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${view}"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_proxyhost.py"
        --module Vfsnative --compiler "$<TARGET_FILE:limbo-bootstrap>"
        --interface "${root}/disVM/module/vfsnative.m" --output "${view}"
    DEPENDS limbo-bootstrap prepare_proxyhost.py "${root}/disVM/module/vfsnative.m" VERBATIM)
add_custom_target(disvm_vfsnative_sources DEPENDS "${view}/vfsnative_decl.h" "${view}/vfsnative_table.h")
add_dependencies(disvm disvm_vfsnative_sources)
target_sources(disvm PRIVATE vfs/provider_module.c vfs/module_adapter.c vfs/vfsnative.c vfs/stack.c vfs/stack_target.c)
list(APPEND module_names vfsctl vfspublish terminal styxscalar scalarowner)
list(APPEND module_sources "${root}/disVM/limbo/vfsctl.b" "${root}/disVM/limbo/vfspublish.b" "${root}/disVM/limbo/terminal.b"
    "${root}/disVM/limbo/styxscalar.b" "${root}/disVM/limbo/scalarowner.b")

# EN/RU: Native embedding SDK; separate from standard Styx/Sys interfaces.
# Native embedding SDK, отдельно от standard Styx/Sys interfaces.
target_sources(disvm PRIVATE vfs/view.c vfs/view_capture.c vfs/view_launch.c vfs/view_io.c vfs/view_list.c vfs/export_native.c vfs/publication.c)

add_custom_command(OUTPUT "${view}/vfsexport_decl.h" "${view}/vfsexport_table.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${view}"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_proxyhost.py"
        --module Vfsexport --compiler "$<TARGET_FILE:limbo-bootstrap>"
        --interface "${root}/disVM/module/vfsexport.m" --output "${view}"
    DEPENDS limbo-bootstrap prepare_proxyhost.py "${root}/disVM/module/vfsexport.m" VERBATIM)
add_custom_target(disvm_vfsexport_sources DEPENDS "${view}/vfsexport_decl.h" "${view}/vfsexport_table.h")
add_dependencies(disvm disvm_vfsexport_sources)
target_sources(disvm PRIVATE vfs/record_pipe.c vfs/cap_record_pipe.c
    vfs/channel_bus.c vfs/channel_proxy.c vfs/channel_route.c vfs/channel_value.c vfs/channel_capability.c
    vfs/returned_proxy.c vfs/result_channels.c)
target_sources(disvm PRIVATE vfs/cap_gc.c vfs/cap_variable.c vfs/cap_array.c vfs/cap_array_index.c vfs/cap_record.c vfs/cap_bigtable.c vfs/cap_tree.c vfs/cap_open.c vfs/cap_publication.c vfs/cap_requests.c vfs/cap_tickets.c vfs/cap_notify.c)
target_sources(disvm PRIVATE vfs/launch/process.c vfs/launch/startup.c vfs/launch/hosted.c vfs/cap_launch.c)
target_sources(disvm PRIVATE vfs/intent.c)

# EN/RU: Automatic job movie endpoint uses scalar VFS/S-record code only, not a second lib9/GUI.
# Автоматический movie endpoint использует только VFS/S-record code, без второй lib9/GUI.
target_sources(disvm PRIVATE
    "${root}/disVM/native/movie/service.c" "${root}/disVM/native/movie/request.c"
    "${root}/disVM/native/movie/protocol.c" "${root}/disVM/native/movie/request_payload.c"
    "${DVM_FLASHVM_SOURCE_ROOT}/src/protocol/srecord.c"
    "${DVM_FLASHVM_SOURCE_ROOT}/src/protocol/sbindings.c")
target_include_directories(disvm PRIVATE "${root}/disVM/native/movie" "${CMAKE_CURRENT_SOURCE_DIR}/vfs" "${DVM_FLASHVM_SOURCE_ROOT}/include")

# EN/RU: Share request-local traversal state with native providers and transport adapters.
# Общий request-local traversal state для native providers и transport adapters.
if(NOT TARGET vfs_walk)
    add_subdirectory("${root}/disVM/host/traversal" "${CMAKE_CURRENT_BINARY_DIR}/walk")
endif()
target_link_libraries(disvm PRIVATE vfs_walk)

target_sources(disvm PRIVATE "${root}/disVM/native/terminal/scalar_verify.c" "${root}/disVM/native/terminal/variable_file.c")

target_sources(disvm PRIVATE vfs/managed_value.c vfs/scalar_call.c vfs/tuple_call.c vfs/ref_call.c vfs/ref_pair_owner.c vfs/scalar_owner.c vfs/loaded_owner.c vfs/dynamic_owner.c vfs/module_shapes.c vfs/scalar_files.c vfs/scalar_direct.c vfs/program_view.c vfs/program_publication.c
    "${root}/disVM/native/terminal/load_profile.c"
    "${root}/disVM/native/terminal/record_write.c"
    "${root}/disVM/native/terminal/scalar_record.c"
    "${root}/disVM/native/terminal/call_record.c"
    "${root}/flashVM/src/capi/utf8.c"
    "${root}/disVM/native/terminal/call_scope.c"
    "${root}/disVM/native/terminal/tuple_record.c"
    "${root}/disVM/native/terminal/ref_pair_record.c"
    "${root}/disVM/native/terminal/scalar_route.c")
