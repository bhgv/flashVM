# Copyright (c) 2026 bhgv
# EN: Explicit non-graphical VFS/FlashVM port inputs; no second lib9 archive.
# RU: Явные неграфические VFS/FlashVM ports; без второй библиотеки lib9.
set(vfs "${INFERNO_VFS_SUPPORT}/disVM/runtime/vfs")
set(ports "${INFERNO_VFS_SUPPORT}/flashVM/ports")
if(NOT EXISTS "${vfs}/cancel.c" OR NOT EXISTS "${vfs}/provider.c" OR NOT EXISTS "${vfs}/provider_capture.c" OR NOT EXISTS "${vfs}/provider_walk.c" OR NOT EXISTS "${vfs}/provider_launch.c" OR NOT EXISTS "${ports}/linux/filesystem.c")
    message(FATAL_ERROR "INFERNO_VFS_SUPPORT must name the flashVM repository root")
endif()
add_library(styx_vfs STATIC file.c tree.c peer.c mount.c mount_launch.c mount_capture.c mount_root.c held_export.c mount_cancel.c mount_io.c mount_notice.c
    feed/control.c feed/worker.c feed/handoff.c control/lifetime.c control/change.c
    events/source.c events/read.c events/provider.c
    subscriptions/lifetime.c subscriptions/open.c
    "${vfs}/cancel.c" "${vfs}/provider.c" "${vfs}/provider_capture.c" "${vfs}/provider_walk.c" "${vfs}/provider_launch.c" "${vfs}/launch/address_guard.c" "${vfs}/handle.c" "${vfs}/io_result.c"
    "${vfs}/changes.c" "${vfs}/changes_refresh.c" "${vfs}/changes_ack.c" "${vfs}/notice.c"
    "${vfs}/publication.c" "${vfs}/view.c" "${vfs}/view_capture.c" "${vfs}/view_launch.c" "${vfs}/view_io.c" "${vfs}/view_list.c" "${vfs}/path.c"
    "${ports}/file_port.c" "${ports}/linux/filesystem.c" "${ports}/linux/channel_sync.c")
target_include_directories(styx_vfs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${vfs}"
    "${ports}" "${INFERNO_VFS_SUPPORT}/flashVM/include" "${INFERNO_VFS_SUPPORT}/disVM/include")
target_link_libraries(styx_vfs PUBLIC Threads::Threads styx_client vfs_walk)
add_executable(vfs-file-wire-test ../tests/provider/backend.c ../tests/provider/wire.c)
set_source_files_properties(../tests/provider/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-file-wire-test PRIVATE styx_vfs infernostyx)
add_test(NAME vfs-file-wire COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/provider/wire.py"
    "$<TARGET_FILE:vfs-file-wire-test>" "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(vfs-file-wire PROPERTIES TIMEOUT 20)

foreach(scenario uncertain readonly held held-retained)
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

add_executable(vfs-mount-test ../tests/mount/client.c ../tests/mount/detached.c ../tests/file-root/context.c)
target_link_libraries(vfs-mount-test PRIVATE styx_namespace_fixture)
add_executable(vfs-mount-deep-server ../tests/mount/deep.c)
set_source_files_properties(../tests/mount/deep.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(vfs-mount-deep-server PRIVATE infernostyx)
foreach(retirement clunk disconnect)
    add_test(NAME vfs-mount-retirement-${retirement} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tests/mount/retirement.py"
        "$<TARGET_FILE:vfs-mount-deep-server>" "${retirement}")
    set_tests_properties(vfs-mount-retirement-${retirement} PROPERTIES TIMEOUT 20)
endforeach()
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
    "${INFERNO_VFS_SUPPORT}/disVM/host/unibus_binding.c"
    "${vfs}/intent.c"
    "${vfs}/launch/movie_offer.c"
    "${vfs}/launch/terminal.c"
    "${vfs}/path.c" "${vfs}/group.c" "${vfs}/mutations.c" "${vfs}/layer_bind.c"
    "${vfs}/union_io.c" "${vfs}/directory.c" "${vfs}/directory_names.c" "${vfs}/auto_access.c"
    "${vfs}/launch/service.c" "${vfs}/launch/service_wait.c" "${vfs}/launch/address.c" "${vfs}/launch/route.c" "${vfs}/launch/admission.c" "${vfs}/launch/ticket.c" "${vfs}/launch/contract.c" "${vfs}/launch/contract_request.c" "${vfs}/launch/accept.c" "${vfs}/launch/prepare.c" "${vfs}/cwd_acquire.c" "${vfs}/resolution.c" "${vfs}/resource.c" "${vfs}/anchor.c" "${vfs}/resource_groups.c"
    "${vfs}/stack.c" "${vfs}/stack_target.c" "${vfs}/export_native.c")
target_link_libraries(styx_namespace_fixture PUBLIC styx_vfs)
# EN/RU: Public native namespace SDK name; existing fixture consumers retain their target.
# Публичное имя native namespace SDK; existing fixture target сохранён.
add_library(styx_namespace ALIAS styx_namespace_fixture)
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

add_executable(vfs-cancel-scope ../tests/cancel/scope.c)
target_link_libraries(vfs-cancel-scope PRIVATE styx_vfs)
add_test(NAME vfs-cancel-scope COMMAND vfs-cancel-scope)
set_tests_properties(vfs-cancel-scope PROPERTIES TIMEOUT 15)
add_executable(vfs-cancel-mount ../tests/cancel/mount.c)
target_link_libraries(vfs-cancel-mount PRIVATE styx_vfs)
foreach(operation read write)
    foreach(mode success cancel race racefail precancel timeout)
        add_test(NAME vfs-cancel-${operation}-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
            "$<TARGET_FILE:vfs-cancel-mount>" "$<TARGET_FILE:control-cancel-server>" "${operation}" "${mode}")
        set_tests_properties(vfs-cancel-${operation}-${mode} PROPERTIES TIMEOUT 30)
    endforeach()
    add_test(NAME vfs-cancel-${operation}-raw COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
        "$<TARGET_FILE:vfs-cancel-mount>" "$<TARGET_FILE:control-cancel-server>" "${operation}" cancel)
    set_tests_properties(vfs-cancel-${operation}-raw PROPERTIES TIMEOUT 30 ENVIRONMENT "STYX_VFS_RAW=1")
endforeach()

add_executable(vfs-cancel-control ../tests/cancel/control.c)
target_link_libraries(vfs-cancel-control PRIVATE styx_vfs)
foreach(operation pathstat pathopen fdstat seek cleanup dirclose capturewalk capturestat)
    foreach(mode success cancel race racefail precancel timeout)
        add_test(NAME vfs-control-${operation}-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
            "$<TARGET_FILE:vfs-cancel-control>" "$<TARGET_FILE:control-cancel-server>" "${operation}" "${mode}")
        set_tests_properties(vfs-control-${operation}-${mode} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()

add_executable(vfs-cancel-directory ../tests/cancel/directory.c)
target_link_libraries(vfs-cancel-directory PRIVATE styx_vfs)
foreach(operation list liststat listprefix)
    foreach(mode success cancel race racefail precancel timeout)
        add_test(NAME vfs-directory-${operation}-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
            "$<TARGET_FILE:vfs-cancel-directory>" "$<TARGET_FILE:control-cancel-server>" "${operation}" "${mode}")
        set_tests_properties(vfs-directory-${operation}-${mode} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
foreach(mode success cancel race racefail precancel timeout)
    add_test(NAME vfs-directory-raw-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
        "$<TARGET_FILE:vfs-cancel-directory>" "$<TARGET_FILE:control-cancel-server>" list "${mode}")
    set_tests_properties(vfs-directory-raw-${mode} PROPERTIES TIMEOUT 30 ENVIRONMENT "STYX_VFS_RAW=1")
endforeach()

add_executable(vfs-cancel-union ../tests/cancel/union.c ../tests/file-root/context.c)
target_link_libraries(vfs-cancel-union PRIVATE styx_namespace_fixture)
add_test(NAME vfs-directory-union-boundary COMMAND vfs-cancel-union)
set_tests_properties(vfs-directory-union-boundary PROPERTIES TIMEOUT 10)

foreach(scenario raw prepare)
    add_test(NAME vfs-directory-truncated-${scenario} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
        "$<TARGET_FILE:vfs-cancel-directory>" "$<TARGET_FILE:control-cancel-server>" list truncated)
    set_tests_properties(vfs-directory-truncated-${scenario} PROPERTIES TIMEOUT 30)
endforeach()
set_tests_properties(vfs-directory-truncated-raw PROPERTIES ENVIRONMENT "STYX_VFS_RAW=1")

foreach(mode success race)
    add_test(NAME vfs-directory-hidden-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
        "$<TARGET_FILE:vfs-cancel-directory>" "$<TARGET_FILE:control-cancel-server>" list "${mode}")
    set_tests_properties(vfs-directory-hidden-${mode} PROPERTIES TIMEOUT 30 ENVIRONMENT "STYX_VFS_HIDE=1")
endforeach()
add_test(NAME vfs-directory-policy-race COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
    "$<TARGET_FILE:vfs-cancel-directory>" "$<TARGET_FILE:control-cancel-server>" list race)
set_tests_properties(vfs-directory-policy-race PROPERTIES TIMEOUT 30 ENVIRONMENT "STYX_VFS_POLICY=1")

add_executable(vfs-cancel-bind ../tests/cancel/bind.c ../tests/file-root/context.c)
target_link_libraries(vfs-cancel-bind PRIVATE styx_namespace_fixture)
add_test(NAME vfs-bind-cancel-transaction COMMAND vfs-cancel-bind)
set_tests_properties(vfs-bind-cancel-transaction PROPERTIES TIMEOUT 10)

add_executable(vfs-launch-service ../tests/launch/service.c ../tests/file-root/context.c)
add_executable(vfs-launch-terminal ../tests/launch/terminal.c ../tests/file-root/context.c
    "${INFERNO_VFS_SUPPORT}/flashVM/src/protocol/srecord.c")
target_include_directories(vfs-launch-terminal PRIVATE "${INFERNO_VFS_SUPPORT}/flashVM/include")
target_link_libraries(vfs-launch-terminal PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-terminal COMMAND vfs-launch-terminal)
set_tests_properties(vfs-launch-terminal PROPERTIES TIMEOUT 10)
add_executable(vfs-code-order ../tests/launch/code_order.c ../tests/file-root/context.c)
target_link_libraries(vfs-code-order PRIVATE styx_namespace_fixture)
add_test(NAME vfs-code-order COMMAND vfs-code-order)
set_tests_properties(vfs-code-order PROPERTIES TIMEOUT 10)
target_link_libraries(vfs-launch-service PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-service PRIVATE "-Wl,--wrap=dvm_launch_request_create" "-Wl,--wrap=dvm_launch_slot_retired")
add_test(NAME vfs-launch-service COMMAND vfs-launch-service)
set_tests_properties(vfs-launch-service PROPERTIES TIMEOUT 15)

add_executable(vfs-launch-ticket-wait ../tests/launch/ticket_wait.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-ticket-wait PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-ticket-wait COMMAND vfs-launch-ticket-wait)
set_tests_properties(vfs-launch-ticket-wait PROPERTIES TIMEOUT 10)
add_executable(vfs-launch-ticket-alloc ../tests/launch/ticket_alloc.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-ticket-alloc PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-ticket-alloc PRIVATE "-Wl,--wrap=malloc" "-Wl,--wrap=calloc" "-Wl,--wrap=free")
add_test(NAME vfs-launch-ticket-alloc COMMAND vfs-launch-ticket-alloc)
set_tests_properties(vfs-launch-ticket-alloc PROPERTIES TIMEOUT 10)

add_executable(vfs-launch-ticket ../tests/launch/ticket.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-ticket PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-ticket COMMAND vfs-launch-ticket)
set_tests_properties(vfs-launch-ticket PROPERTIES TIMEOUT 10)

add_executable(vfs-launch-prepare ../tests/launch/prepare.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-prepare PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-prepare COMMAND vfs-launch-prepare)
set_tests_properties(vfs-launch-prepare PROPERTIES TIMEOUT 10)

add_executable(vfs-provider-cwd ../tests/cancel/cwd.c ../tests/file-root/context.c)
target_link_libraries(vfs-provider-cwd PRIVATE styx_namespace_fixture)
add_test(NAME vfs-provider-cwd COMMAND vfs-provider-cwd)
set_tests_properties(vfs-provider-cwd PROPERTIES TIMEOUT 10)

add_executable(vfs-cancel-move ../tests/cancel/move.c ../tests/file-root/context.c)
target_link_libraries(vfs-cancel-move PRIVATE styx_namespace_fixture)
add_test(NAME vfs-move-cancel-transaction COMMAND vfs-cancel-move)
set_tests_properties(vfs-move-cancel-transaction PROPERTIES TIMEOUT 10)

add_executable(vfs-cancel-close ../tests/cancel/close.c
    "${INFERNO_VFS_SUPPORT}/disVM/runtime/close_worker.c" "${ports}/linux/thread.c")
target_include_directories(vfs-cancel-close PRIVATE "${INFERNO_VFS_SUPPORT}/disVM/runtime")
target_link_libraries(vfs-cancel-close PRIVATE styx_vfs)
target_link_libraries(vfs-cancel-close PRIVATE "-Wl,--wrap=dvm_cancel_create" "-Wl,--wrap=dvm_cancel_request" "-Wl,--wrap=fvm_port_thread_start")
foreach(mode active replay)
    add_test(NAME vfs-close-cancel-${mode} COMMAND vfs-cancel-close "${mode}")
    set_tests_properties(vfs-close-cancel-${mode} PROPERTIES TIMEOUT 10)
endforeach()

add_executable(vfs-cancel-close-queue ../tests/cancel/close_queue.c
    "${INFERNO_VFS_SUPPORT}/disVM/runtime/close_worker.c" "${ports}/linux/thread.c")
target_include_directories(vfs-cancel-close-queue PRIVATE "${INFERNO_VFS_SUPPORT}/disVM/runtime")
target_link_libraries(vfs-cancel-close-queue PRIVATE styx_vfs)
target_link_libraries(vfs-cancel-close-queue PRIVATE "-Wl,--wrap=dvm_vfs_close")
foreach(mode success cancel race racefail)
    add_test(NAME vfs-close-queue-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/control-cancel/wire.py"
        "$<TARGET_FILE:vfs-cancel-close-queue>" "$<TARGET_FILE:control-cancel-server>" queueclose "${mode}")
    set_tests_properties(vfs-close-queue-${mode} PROPERTIES TIMEOUT 30)
endforeach()

add_test(NAME system-installation-identity COMMAND "${PYTHON_EXECUTABLE}" "${INFERNO_VFS_SUPPORT}/disVM/runtime/test_system_identity.py")
set_tests_properties(system-installation-identity PROPERTIES TIMEOUT 30)

# EN/RU: Pure native launch capability, independent of VM/renderer linkage.
# Чистая native-capability запуска без линковки VM/renderer.
add_executable(unibus-binding-test "${INFERNO_VFS_SUPPORT}/disVM/tests/unibus_binding.c")
target_link_libraries(unibus-binding-test PRIVATE styx_namespace_fixture)
add_test(NAME native-unibus-binding COMMAND unibus-binding-test)

# EN/RU: Blocking native admission receives one request; cancellation never closes the shared service.
# Blocking native admission получает один request; отмена не закрывает общий service.
add_executable(vfs-launch-service-wait ../tests/launch/wait/main.c ../tests/launch/wait/gates.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-service-wait PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-service-wait PRIVATE "-Wl,--wrap=fvm_port_sync_wait" "-Wl,--wrap=dvm_launch_service_next")
add_test(NAME vfs-launch-service-wait COMMAND vfs-launch-service-wait)
set_tests_properties(vfs-launch-service-wait PROPERTIES TIMEOUT 20)
add_executable(vfs-launch-service-wait-alloc ../tests/launch/wait/allocation.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-service-wait-alloc PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-service-wait-alloc PRIVATE "-Wl,--wrap=malloc" "-Wl,--wrap=calloc" "-Wl,--wrap=free")
add_test(NAME vfs-launch-service-wait-alloc COMMAND vfs-launch-service-wait-alloc)
set_tests_properties(vfs-launch-service-wait-alloc PROPERTIES TIMEOUT 10)
add_executable(vfs-launch-service-wait-detached ../tests/launch/wait/detached.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-service-wait-detached PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-service-wait-detached PRIVATE "-Wl,--wrap=dvm_notice_subscribe" "-Wl,--wrap=fvm_port_sync_wait" "-Wl,--wrap=free")
add_test(NAME vfs-launch-service-wait-detached COMMAND vfs-launch-service-wait-detached)
set_tests_properties(vfs-launch-service-wait-detached PROPERTIES TIMEOUT 10)

# EN/RU: Typed module lookup is local to each caller; explicit asset intents keep their existing policies.
# Поиск typed module локален каждому caller; explicit asset intents сохраняют свои policies.
add_executable(vfs-call-order ../tests/launch/call_order.c ../tests/file-root/context.c)
target_link_libraries(vfs-call-order PRIVATE styx_namespace_fixture)
add_test(NAME vfs-call-order COMMAND vfs-call-order)
set_tests_properties(vfs-call-order PROPERTIES TIMEOUT 15)

# EN/RU: Pinned VFS source to capability-bound launch destination, no code download or inferred peer authority.
# Pinned VFS source в capability-bound launch destination, без code download и inferred peer authority.
add_executable(vfs-launch-route ../tests/launch/route/main.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-route PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-route COMMAND vfs-launch-route)
set_tests_properties(vfs-launch-route PROPERTIES TIMEOUT 15)
add_executable(vfs-launch-route-race ../tests/launch/route/race.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-route-race PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-route-race PRIVATE "-Wl,--wrap=dvm_launch_service_submit")
add_test(NAME vfs-launch-route-race COMMAND vfs-launch-route-race)
set_tests_properties(vfs-launch-route-race PROPERTIES TIMEOUT 15)
add_executable(vfs-launch-route-alloc ../tests/launch/route/allocation.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-route-alloc PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-route-alloc PRIVATE "-Wl,--wrap=malloc" "-Wl,--wrap=calloc" "-Wl,--wrap=free")
add_test(NAME vfs-launch-route-alloc COMMAND vfs-launch-route-alloc)
set_tests_properties(vfs-launch-route-alloc PROPERTIES TIMEOUT 15)

# EN/RU: Provider-advertised addresses preserve capture/export scopes and live execution policy after discovery.
# Provider-advertised addresses сохраняют capture/export scope и live execution policy после discovery.
add_executable(vfs-launch-discovery ../tests/launch/discovery/main.c ../tests/launch/discovery/source.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-discovery PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-discovery COMMAND vfs-launch-discovery)
set_tests_properties(vfs-launch-discovery PROPERTIES TIMEOUT 20)
add_executable(vfs-launch-discovery-alloc ../tests/launch/discovery/allocation.c ../tests/launch/discovery/source.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-discovery-alloc PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-discovery-alloc PRIVATE "-Wl,--wrap=malloc" "-Wl,--wrap=calloc" "-Wl,--wrap=free")
add_test(NAME vfs-launch-discovery-alloc COMMAND vfs-launch-discovery-alloc)
set_tests_properties(vfs-launch-discovery-alloc PROPERTIES TIMEOUT 15)

# EN/RU: Deferred reusable resolution reserves no queue quota and never re-resolves a later bind.
# Отложенный reusable resolve не резервирует quota и не разрешает более поздний bind повторно.
add_executable(vfs-launch-resolve ../tests/launch/discovery/resolve.c ../tests/launch/discovery/source.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-resolve PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-resolve COMMAND vfs-launch-resolve)
set_tests_properties(vfs-launch-resolve PROPERTIES TIMEOUT 20)
add_test(NAME vfs-launch-resolve-alloc COMMAND vfs-launch-discovery-alloc resolve)
set_tests_properties(vfs-launch-resolve-alloc PROPERTIES TIMEOUT 15)

# EN/RU: Inferno name/signature contracts must match before any provider environment acquisition.
# Контракты имён/сигнатур Inferno должны совпасть до получения provider environment.
add_executable(vfs-launch-contract ../tests/launch/contract/main.c ../tests/file-root/context.c)
target_link_libraries(vfs-launch-contract PRIVATE styx_namespace_fixture)
add_test(NAME vfs-launch-contract COMMAND vfs-launch-contract)
set_tests_properties(vfs-launch-contract PROPERTIES TIMEOUT 15)
add_test(NAME vfs-launch-contract-alloc COMMAND vfs-launch-ticket-alloc typed)
set_tests_properties(vfs-launch-contract-alloc PROPERTIES TIMEOUT 15)

# EN/RU: Typed requirements survive request/take, independent context copying and parent retirement.
# Typed requirements переживают request/take, independent context copying и retirement parent.
add_executable(vfs-launch-contract-context ../tests/launch/contract/context.c "${vfs}/launch/context.c" ../tests/file-root/context.c)
target_link_libraries(vfs-launch-contract-context PRIVATE styx_namespace_fixture)
target_link_libraries(vfs-launch-contract-context PRIVATE "-Wl,--wrap=malloc" "-Wl,--wrap=free")
add_test(NAME vfs-launch-contract-context COMMAND vfs-launch-contract-context)
set_tests_properties(vfs-launch-contract-context PROPERTIES TIMEOUT 15)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../tests/traversal" "${CMAKE_CURRENT_BINARY_DIR}/traversal-tests")
