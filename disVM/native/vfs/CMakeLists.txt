# Copyright (c) 2026 bhgv
# EN: Explicit non-graphical VFS/FlashVM port inputs; no second lib9 archive.
# RU: Явные неграфические VFS/FlashVM ports; без второй библиотеки lib9.
set(vfs "${INFERNO_VFS_SUPPORT}/disVM/runtime/vfs")
set(ports "${INFERNO_VFS_SUPPORT}/flashVM/ports")
if(NOT EXISTS "${vfs}/provider.c" OR NOT EXISTS "${ports}/linux/filesystem.c")
    message(FATAL_ERROR "INFERNO_VFS_SUPPORT must name the flashVM repository root")
endif()
add_library(styx_vfs STATIC file.c tree.c peer.c mount.c mount_io.c
    "${vfs}/provider.c" "${vfs}/handle.c" "${vfs}/io_result.c"
    "${vfs}/changes.c" "${vfs}/changes_refresh.c" "${vfs}/changes_ack.c" "${vfs}/notice.c"
    "${vfs}/publication.c" "${vfs}/view.c" "${vfs}/view_io.c" "${vfs}/view_list.c" "${vfs}/path.c"
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
