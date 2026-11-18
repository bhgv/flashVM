# Copyright (c) 2026 bhgv
# EN: Original 9P peers exercise the autonomous production VFS server and isolated TX/native lifecycle faults.
# RU: Исходные 9P peers проверяют production VFS server и изолированные TX/native lifecycle faults.
set_source_files_properties(tests/server_host.c tests/server_wire.c tests/server_fault.c PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/server_wire.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(server_case RANGE 0 11)
    set(server_peer terminal-server-peer-${server_case})
    add_executable(${server_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/server_host.c tests/server_wire.c tests/server_fault.c tests/exchange_provider.c)
    target_compile_definitions(${server_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_SERVER_CASE=${server_case})
    target_link_libraries(${server_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    if(server_case EQUAL 6 OR server_case EQUAL 7)
        set_property(TARGET ${server_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwritecancel")
    elseif(server_case EQUAL 9)
        set_property(TARGET ${server_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=pthread_create")
    endif()
    foreach(server_transport normal cancel)
        add_test(NAME terminal-server-${server_case}-${server_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${server_peer}>" ${server_transport})
        set_tests_properties(terminal-server-${server_case}-${server_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()

add_executable(terminal-send-wait tests/send_wait.c)
target_link_libraries(terminal-send-wait PRIVATE styx_terminal)
target_compile_options(terminal-send-wait PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_property(TARGET terminal-send-wait APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=dvm_cancel_subscribe")
foreach(wait_case cancel space timeout pre invalid)
    add_test(NAME terminal-send-wait-${wait_case} COMMAND terminal-send-wait ${wait_case})
    set_tests_properties(terminal-send-wait-${wait_case} PROPERTIES TIMEOUT 10)
endforeach()
