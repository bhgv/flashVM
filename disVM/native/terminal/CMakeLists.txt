# Copyright (c) 2026 bhgv
# EN: Generic native terminal record transport reuses original FlashVM codecs, no graphics.
# RU: Generic native terminal record transport использует FlashVM codecs, без графики.
add_library(styx_terminal STATIC life.c queue.c send_wait.c io.c "${INFERNO_VFS_SUPPORT}/flashVM/src/protocol/sstream.c")
target_include_directories(styx_terminal PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_terminal PUBLIC styx_launch styx_movie_protocol)
target_compile_options(styx_terminal PRIVATE -Wall -Wextra)

# EN/RU: Reuse the standalone IDE value codec; no FlashVM VM or graphics objects.
# Общий кодек значений IDE без VM и графических объектов FlashVM.
add_library(styx_terminal_ui STATIC ui/frame.c "${INFERNO_VFS_SUPPORT}/disVM/host/ui_frame.c")
target_include_directories(styx_terminal_ui PUBLIC "${INFERNO_VFS_SUPPORT}/disVM/include")
target_link_libraries(styx_terminal_ui PUBLIC styx_terminal)
target_compile_options(styx_terminal_ui PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_library(styx_terminal_route STATIC route/life.c route/dispatch.c route/queue.c route/types.c route/wait.c route/retirement.c)
target_link_libraries(styx_terminal_route PUBLIC styx_terminal_ui)
target_compile_options(styx_terminal_route PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_subdirectory(bridge)
add_library(styx_terminal_resource STATIC resource/life.c resource/fetch.c resource/budget.c
    resource/server_life.c resource/server_state.c resource/server_run.c resource/server_retirement.c
    resource/announced_life.c resource/announced_run.c resource/announced_state.c resource/announced_fetch.c
    resource/outgoing_life.c resource/outgoing_run.c resource/outgoing_state.c)
target_link_libraries(styx_terminal_resource PUBLIC styx_terminal_route styx_movie_service styx_movie_client)
target_compile_options(styx_terminal_resource PRIVATE -Wall -Wextra -Wpedantic -Werror)
include(tests/exchange.cmake)
include(tests/server.cmake)
include(tests/offers.cmake)
include(tests/publisher.cmake)
include(tests/retirement/cases.cmake)
include(tests/active_io/cases.cmake)

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

# EN/RU: Existing resource replies use the same validator through a typed queue and ordinary Styx.
# Существующие ответы ресурса используют одну проверку через typed queue и обычный Styx.
set_source_files_properties(tests/resource_reply.c PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
foreach(reply_case RANGE 0 4)
    set(reply_peer terminal-resource-peer-${reply_case})
    add_executable(${reply_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/typed_server.c tests/resource_reply.c)
    target_compile_definitions(${reply_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_REPLY_ROUTE_TEST=${reply_case})
    target_link_libraries(${reply_peer} PRIVATE styx_terminal_route infernostyx_phy remote_phy_mux)
    foreach(reply_transport normal cancel)
        add_test(NAME terminal-resource-${reply_case}-${reply_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${reply_peer}>" ${reply_transport})
        set_tests_properties(terminal-resource-${reply_case}-${reply_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()

# EN/RU: Generic typed records coexist with UI and unknown records over the original fragmented 9P.
# Общие типизированные записи рядом с UI и неизвестными записями через исходный fragmented 9P.
set_source_files_properties(tests/typed_server.c tests/typed_route.c PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
# EN/RU: Original Inferno public headers retain Plan 9 pragmas; do not edit the imported declarations.
# Исходные заголовки Inferno сохраняют pragmas Plan 9; импортированные объявления не менять.
set_property(SOURCE tests/typed_server.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(typed_case RANGE 0 6)
    set(typed_peer terminal-typed-peer-${typed_case})
    add_executable(${typed_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/typed_server.c tests/typed_route.c)
    target_compile_definitions(${typed_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_TYPED_CASE=${typed_case})
    target_link_libraries(${typed_peer} PRIVATE styx_terminal_route infernostyx_phy remote_phy_mux)
    foreach(typed_transport normal cancel)
        add_test(NAME terminal-typed-${typed_case}-${typed_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${typed_peer}>" ${typed_transport})
        set_tests_properties(terminal-typed-${typed_case}-${typed_transport} PROPERTIES TIMEOUT 30)
    endforeach()
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
