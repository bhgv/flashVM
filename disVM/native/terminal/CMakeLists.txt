# Copyright (c) 2026 bhgv
# EN: Generic native terminal record transport reuses original FlashVM codecs, no graphics.
# RU: Generic native terminal record transport использует FlashVM codecs, без графики.
add_library(styx_terminal STATIC life.c queue.c io.c "${INFERNO_VFS_SUPPORT}/flashVM/src/protocol/sstream.c")
target_include_directories(styx_terminal PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_terminal PUBLIC styx_launch styx_movie_protocol)
target_compile_options(styx_terminal PRIVATE -Wall -Wextra)

# EN/RU: Reuse the standalone IDE value codec; no FlashVM VM or graphics objects.
# Общий кодек значений IDE без VM и графических объектов FlashVM.
add_library(styx_terminal_ui STATIC ui/frame.c "${INFERNO_VFS_SUPPORT}/disVM/host/ui_frame.c")
target_include_directories(styx_terminal_ui PUBLIC "${INFERNO_VFS_SUPPORT}/disVM/include")
target_link_libraries(styx_terminal_ui PUBLIC styx_terminal)
target_compile_options(styx_terminal_ui PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_library(styx_terminal_route STATIC route/life.c route/dispatch.c route/queue.c)
target_link_libraries(styx_terminal_route PUBLIC styx_terminal_ui)
target_compile_options(styx_terminal_route PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_subdirectory(bridge)

add_executable(terminal-ui-codec "${INFERNO_VFS_SUPPORT}/disVM/tests/ui_frame.c")
target_link_libraries(terminal-ui-codec PRIVATE styx_terminal_ui)
add_test(NAME terminal-ui-codec COMMAND terminal-ui-codec)

add_executable(terminal-record-peer ../tests/phy-mux/peer.c ../tests/phy/limited.c
    ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c tests/server.c tests/client.c)
target_compile_definitions(terminal-record-peer PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
    STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1)
target_link_libraries(terminal-record-peer PRIVATE styx_terminal_ui infernostyx_phy remote_phy_mux)
foreach(terminal_case normal cancel)
    add_test(NAME terminal-record-${terminal_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:terminal-record-peer>" ${terminal_case})
    set_tests_properties(terminal-record-${terminal_case} PROPERTIES TIMEOUT 30)
endforeach()

add_executable(terminal-record-fault tests/faults.c)
target_link_libraries(terminal-record-fault PRIVATE styx_terminal)
set_property(TARGET terminal-record-fault APPEND_STRING PROPERTY LINK_FLAGS
    " -Wl,--wrap=styxclientreadwatch -Wl,--wrap=styxclientwritecancel")
foreach(record_fault crc truncated limit batch short uncertain)
    add_test(NAME terminal-record-${record_fault} COMMAND terminal-record-fault ${record_fault})
    set_tests_properties(terminal-record-${record_fault} PROPERTIES TIMEOUT 10)
endforeach()

# EN/RU: Real handoff peers exercise autonomous routing and terminal queue failure paths.
# Настоящий handoff проверяет автономную маршрутизацию и завершение очередей.
foreach(route_case RANGE 0 6)
    set(route_peer terminal-route-peer-${route_case})
    add_executable(${route_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/server.c tests/route.c)
    target_compile_definitions(${route_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_ROUTE_CASE=${route_case})
    target_link_libraries(${route_peer} PRIVATE styx_terminal_route infernostyx_phy remote_phy_mux)
    foreach(route_transport normal cancel)
        add_test(NAME terminal-route-${route_case}-${route_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${route_peer}>" ${route_transport})
        set_tests_properties(terminal-route-${route_case}-${route_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
