# Copyright (c) 2026 bhgv
# EN: Explicit non-graphical VFS/FlashVM port inputs; no second lib9 archive.
# RU: Явные неграфические VFS/FlashVM ports; без второй библиотеки lib9.
set(vfs "${INFERNO_VFS_SUPPORT}/disVM/runtime/vfs")
set(ports "${INFERNO_VFS_SUPPORT}/flashVM/ports")
if(NOT EXISTS "${vfs}/provider.c" OR NOT EXISTS "${vfs}/provider_capture.c" OR NOT EXISTS "${ports}/linux/filesystem.c")
    message(FATAL_ERROR "INFERNO_VFS_SUPPORT must name the flashVM repository root")
endif()
add_library(styx_vfs STATIC file.c tree.c peer.c mount.c mount_capture.c mount_root.c mount_io.c mount_notice.c
    feed/control.c feed/worker.c feed/handoff.c control/lifetime.c control/change.c
    events/source.c events/read.c events/provider.c
    subscriptions/lifetime.c subscriptions/open.c
    "${vfs}/provider.c" "${vfs}/provider_capture.c" "${vfs}/handle.c" "${vfs}/io_result.c"
    "${vfs}/changes.c" "${vfs}/changes_refresh.c" "${vfs}/changes_ack.c" "${vfs}/notice.c"
    "${vfs}/publication.c" "${vfs}/view.c" "${vfs}/view_capture.c" "${vfs}/view_io.c" "${vfs}/view_list.c" "${vfs}/path.c"
    "${ports}/file_port.c" "${ports}/linux/filesystem.c" "${ports}/linux/channel_sync.c")
target_include_directories(styx_vfs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${vfs}"
    "${ports}" "${INFERNO_VFS_SUPPORT}/flashVM/include" "${INFERNO_VFS_SUPPORT}/disVM/include")
target_link_libraries(styx_vfs PUBLIC Threads::Threads styx_client)
add_executable(vfs-file-wire-test ../tests/provider/backend.c ../tests/provider/wire.c)
set_source_files_properties(../tests/provider/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-file-wire-test PRIVATE styx_vfs infernostyx)
add_test(NAME vfs-file-wire COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/provider/wire.py"
    "$<TARGET_FILE:vfs-file-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-file-wire PROPERTIES TIMEOUT 20)

foreach(scenario uncertain readonly)
    add_test(NAME vfs-file-wire-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/provider/wire.py"
        "$<TARGET_FILE:vfs-file-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-file-wire-${scenario} PROPERTIES TIMEOUT 20)
endforeach()
add_executable(vfs-tree-wire-test ../tests/tree/backend.c ../tests/tree/wire.c)
set_source_files_properties(../tests/tree/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-tree-wire-test PRIVATE styx_vfs infernostyx)
add_test(NAME vfs-tree-wire COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/tree/wire.py"
    "$<TARGET_FILE:vfs-tree-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-tree-wire PROPERTIES TIMEOUT 30)

add_executable(vfs-peer-wire-test ../tests/peer/backend.c ../tests/peer/wire.c ../tests/peer/review.c)
set_source_files_properties(../tests/peer/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-peer-wire-test PRIVATE styx_vfs infernostyx)
add_test(NAME vfs-peer-wire COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/peer/wire.py"
    "$<TARGET_FILE:vfs-peer-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-peer-wire PROPERTIES TIMEOUT 30)

add_test(NAME vfs-peer-deferred-wire COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/peer/deferred.py"
    "$<TARGET_FILE:vfs-peer-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-peer-deferred-wire PROPERTIES TIMEOUT 30)
add_test(NAME vfs-peer-deferred-stop-wire COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/peer/deferred.py"
    "$<TARGET_FILE:vfs-peer-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests" shutdown)
set_tests_properties(vfs-peer-deferred-stop-wire PROPERTIES TIMEOUT 30)

add_executable(vfs-mount-test ../tests/mount/client.c)
target_link_libraries(vfs-mount-test PRIVATE styx_vfs)
add_executable(vfs-mount-deep-server ../tests/mount/deep.c)
set_source_files_properties(../tests/mount/deep.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-mount-deep-server PRIVATE infernostyx)
foreach(scenario basic partial loss badtag badcount msize iounit dirbad readonly deep slow badwalk)
    if(scenario STREQUAL "partial")
        set(mount_server "$<TARGET_FILE:vfs-file-wire-test>")
    elseif(scenario STREQUAL "deep")
        set(mount_server "$<TARGET_FILE:vfs-mount-deep-server>")
    else()
        set(mount_server "$<TARGET_FILE:server-loopback-test>")
    endif()
    add_test(NAME vfs-mount-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/mount/wire.py"
        "$<TARGET_FILE:vfs-mount-test>" "${mount_server}" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-mount-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

add_test(NAME vfs-mount-prepared COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/mount/prepared.py"
    "$<TARGET_FILE:vfs-mount-test>" "$<TARGET_FILE:vfs-peer-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-mount-prepared PROPERTIES TIMEOUT 25)

foreach(scenario basic partial loss badtag badcount msize iounit dirbad readonly deep slow badwalk)
    if(scenario STREQUAL "partial")
        set(mux_server "$<TARGET_FILE:vfs-file-wire-test>")
    elseif(scenario STREQUAL "deep")
        set(mux_server "$<TARGET_FILE:vfs-mount-deep-server>")
    else()
        set(mux_server "$<TARGET_FILE:server-loopback-test>")
    endif()
    add_test(NAME vfs-mux-provider-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/mount/wire.py"
        "$<TARGET_FILE:vfs-mount-test>" "${mux_server}" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-mux-provider-${scenario} PROPERTIES TIMEOUT 25 ENVIRONMENT "STYX_NATIVE_MUX=1")
endforeach()

add_executable(vfs-mux-test ../tests/mux/client.c)
target_link_libraries(vfs-mux-test PRIVATE styx_vfs)
foreach(scenario reorder quota cancel race timeout noack failall precancel writecancel writerace detach shutdown late badflush racefail finaldrop)
    add_test(NAME vfs-mux-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/mux/wire.py"
        "$<TARGET_FILE:vfs-mux-test>" "$<TARGET_FILE:server-loopback-test>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-mux-${scenario} PROPERTIES TIMEOUT 25)
endforeach()
add_test(NAME vfs-mux-provider-prepared COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/mount/prepared.py"
    "$<TARGET_FILE:vfs-mount-test>" "$<TARGET_FILE:vfs-peer-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-mux-provider-prepared PROPERTIES TIMEOUT 25 ENVIRONMENT "STYX_NATIVE_MUX=1")

add_executable(vfs-idle-events-test ../tests/events/client.c)
target_link_libraries(vfs-idle-events-test PRIVATE styx_vfs)
foreach(scenario idle shared shared-detach shutdown detached drop inflight late)
    add_test(NAME vfs-idle-events-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/events/wire.py"
        "$<TARGET_FILE:vfs-idle-events-test>" "$<TARGET_FILE:server-loopback-test>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-idle-events-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

add_executable(vfs-heartbeat-test ../tests/events/heartbeat.c)
target_link_libraries(vfs-heartbeat-test PRIVATE styx_vfs)
foreach(scenario healthy pending recover traffic timeout disable reconfigure badreply)
    add_test(NAME vfs-heartbeat-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/events/heartbeat.py"
        "$<TARGET_FILE:vfs-heartbeat-test>" "$<TARGET_FILE:server-loopback-test>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-heartbeat-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

if(TARGET server-deferred-read-tcp)
    add_executable(vfs-deferred-read-client ../tests/events/deferred.c)
    target_link_libraries(vfs-deferred-read-client PRIVATE styx_vfs)
    foreach(scenario deliver cancel)
        add_test(NAME vfs-deferred-read-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/events/deferred.py"
            "$<TARGET_FILE:vfs-deferred-read-client>" "$<TARGET_FILE:server-deferred-read-tcp>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
        set_tests_properties(vfs-deferred-read-${scenario} PROPERTIES TIMEOUT 25)
    endforeach()
endif()

add_executable(vfs-event-resource-queue ../tests/event-resource/queue.c)
target_link_libraries(vfs-event-resource-queue PRIVATE styx_vfs)
add_test(NAME vfs-event-resource-queue COMMAND vfs-event-resource-queue)
set_tests_properties(vfs-event-resource-queue PROPERTIES TIMEOUT 10)

add_executable(vfs-event-resource-race ../tests/event-resource/race.c)
target_link_libraries(vfs-event-resource-race PRIVATE styx_vfs)
add_test(NAME vfs-event-resource-race COMMAND vfs-event-resource-race)
set_tests_properties(vfs-event-resource-race PROPERTIES TIMEOUT 10)

add_executable(vfs-event-resource-server ../tests/event-resource/backend.c ../tests/event-resource/wire.c)
set_source_files_properties(../tests/event-resource/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-event-resource-server PRIVATE styx_vfs infernostyx)

foreach(scenario normal flush reopen finish end)
    add_test(NAME vfs-event-resource-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/event-resource/wire.py"
        "$<TARGET_FILE:vfs-event-resource-server>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-event-resource-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

add_executable(vfs-event-resource-client ../tests/event-resource/client.c)
target_link_libraries(vfs-event-resource-client PRIVATE styx_vfs)
foreach(scenario deliver cancel)
    add_test(NAME vfs-event-resource-client-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/event-resource/client.py"
        "$<TARGET_FILE:vfs-event-resource-client>" "$<TARGET_FILE:vfs-event-resource-server>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-event-resource-client-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

add_executable(vfs-feed-journal ../tests/feed/journal.c)
target_link_libraries(vfs-feed-journal PRIVATE styx_vfs)
add_test(NAME vfs-feed-journal COMMAND vfs-feed-journal)
set_tests_properties(vfs-feed-journal PROPERTIES TIMEOUT 10)

add_executable(vfs-feed-parser ../tests/feed/parser.c)
target_link_libraries(vfs-feed-parser PRIVATE styx_vfs)
add_test(NAME vfs-feed-parser COMMAND vfs-feed-parser)
set_tests_properties(vfs-feed-parser PROPERTIES TIMEOUT 10)

add_executable(vfs-feed-client ../tests/feed/client.c)
target_link_libraries(vfs-feed-client PRIVATE styx_vfs)
foreach(scenario normal stop inflight shutdown bad progress partial badpath noslots writeonly)
    add_test(NAME vfs-feed-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/feed/wire.py"
        "$<TARGET_FILE:vfs-feed-client>" "$<TARGET_FILE:vfs-event-resource-server>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-feed-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

add_executable(vfs-mount-control ../tests/control/client.c)
target_link_libraries(vfs-mount-control PRIVATE styx_vfs)
foreach(scenario replace chain clear setrace closerace inflight empty ceiling)
    add_test(NAME vfs-mount-control-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control/wire.py"
        "$<TARGET_FILE:vfs-mount-control>" "$<TARGET_FILE:vfs-event-resource-server>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-mount-control-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

add_executable(vfs-subscriptions-queue ../tests/subscriptions/queue.c)
target_link_libraries(vfs-subscriptions-queue PRIVATE styx_vfs)
foreach(scenario cache overflow quota close rollback empty)
    add_test(NAME vfs-subscriptions-${scenario} COMMAND vfs-subscriptions-queue "${scenario}")
    set_tests_properties(vfs-subscriptions-${scenario} PROPERTIES TIMEOUT 10)
endforeach()
add_executable(vfs-subscriptions-race ../tests/subscriptions/race.c)
target_link_libraries(vfs-subscriptions-race PRIVATE styx_vfs)
foreach(scenario setup retirement quota)
    add_test(NAME vfs-subscriptions-race-${scenario} COMMAND vfs-subscriptions-race "${scenario}")
    set_tests_properties(vfs-subscriptions-race-${scenario} PROPERTIES TIMEOUT 10)
endforeach()
foreach(scenario normal short flush reopen)
    add_test(NAME vfs-subscriptions-wire-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/subscriptions/wire.py"
        "$<TARGET_FILE:vfs-event-resource-server>" "${INFERNO_SOURCE}/tools/libstyx/tests" "${scenario}")
    set_tests_properties(vfs-subscriptions-wire-${scenario} PROPERTIES TIMEOUT 25)
endforeach()
add_test(NAME vfs-subscriptions-mounted COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/subscriptions/mounted.py"
    "$<TARGET_FILE:vfs-feed-client>" "$<TARGET_FILE:vfs-event-resource-server>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-subscriptions-mounted PROPERTIES TIMEOUT 25)

# EN/RU: Standalone native namespace SDK fixture; explicit repository sources, no sibling build dependency.
# Standalone native namespace SDK fixture; явные sources репозитория, без sibling build dependency.
add_library(styx_namespace_fixture STATIC
    "${vfs}/path.c" "${vfs}/group.c" "${vfs}/mutations.c" "${vfs}/layer_bind.c"
    "${vfs}/union_io.c" "${vfs}/directory.c" "${vfs}/directory_names.c" "${vfs}/auto_access.c"
    "${vfs}/resolution.c" "${vfs}/resource.c" "${vfs}/anchor.c" "${vfs}/resource_groups.c"
    "${vfs}/stack.c" "${vfs}/stack_target.c" "${vfs}/export_native.c")
target_link_libraries(styx_namespace_fixture PUBLIC styx_vfs)
add_executable(vfs-file-root-server ../tests/file-root/backend.c ../tests/file-root/wire.c ../tests/peer/review.c)
set_source_files_properties(../tests/file-root/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-file-root-server PRIVATE styx_vfs infernostyx)
add_executable(vfs-file-root-client ../tests/file-root/client.c ../tests/file-root/context.c)
target_link_libraries(vfs-file-root-client PRIVATE styx_namespace_fixture)
foreach(scenario direct prepare)
    add_test(NAME vfs-file-root-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/file-root/wire.py"
        "$<TARGET_FILE:vfs-file-root-client>" "$<TARGET_FILE:vfs-file-root-server>" "${scenario}")
    set_tests_properties(vfs-file-root-${scenario} PROPERTIES TIMEOUT 25)
endforeach()

add_executable(vfs-provider-capture ../tests/subroot/capture.c ../tests/file-root/context.c)
target_link_libraries(vfs-provider-capture PRIVATE styx_namespace_fixture)
add_test(NAME vfs-provider-capture COMMAND vfs-provider-capture)
set_tests_properties(vfs-provider-capture PROPERTIES TIMEOUT 10)
add_executable(vfs-subroot-server ../tests/subroot/server.c)
set_source_files_properties(../tests/subroot/server.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-subroot-server PRIVATE infernostyx)
add_executable(vfs-subroot-client ../tests/subroot/client.c ../tests/file-root/context.c)
target_link_libraries(vfs-subroot-client PRIVATE styx_namespace_fixture)
foreach(scenario bind at)
    add_test(NAME vfs-subroot-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/subroot/wire.py"
        "$<TARGET_FILE:vfs-subroot-client>" "$<TARGET_FILE:vfs-subroot-server>" "${scenario}")
    set_tests_properties(vfs-subroot-${scenario} PROPERTIES TIMEOUT 30)
endforeach()
add_executable(vfs-subroot-retire ../tests/subroot/retire.c mount_root.c)
target_link_libraries(vfs-subroot-retire PRIVATE styx_vfs)
add_test(NAME vfs-subroot-retire COMMAND vfs-subroot-retire)
set_tests_properties(vfs-subroot-retire PROPERTIES TIMEOUT 10)

include("${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/cases.cmake")
