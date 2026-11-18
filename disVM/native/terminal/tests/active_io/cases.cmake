# Copyright (c) 2026 bhgv
# EN: Active native provider scopes cancel on route retirement, isolated from another scope on the same factory.
# RU: Active native provider scopes отменяются при route retirement независимо от другого scope того же factory.
set_source_files_properties(tests/active_io/host.c tests/active_io/provider.c tests/active_io/gate.c tests/active_io/fault.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
foreach(active_io_case RANGE 0 15)
    set(active_io_peer terminal-active-io-peer-${active_io_case})
    add_executable(${active_io_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/server_wire.c tests/server_fault.c tests/active_io/host.c tests/active_io/provider.c
        tests/active_io/gate.c tests/active_io/fault.c)
    target_compile_definitions(${active_io_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_ACTIVE_IO_CASE=${active_io_case})
    set_property(TARGET ${active_io_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxbidirroutewatch")
    if(active_io_case LESS 4)
        target_compile_definitions(${active_io_peer} PRIVATE STYX_SERVER_CASE=7)
        set_property(TARGET ${active_io_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwritecancel")
    elseif(active_io_case EQUAL 14)
        target_compile_definitions(${active_io_peer} PRIVATE STYX_SERVER_CASE=9)
        set_property(TARGET ${active_io_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=pthread_create")
    endif()
    target_link_libraries(${active_io_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    foreach(active_io_transport normal cancel)
        add_test(NAME terminal-active-io-${active_io_case}-${active_io_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${active_io_peer}>" ${active_io_transport})
        set_tests_properties(terminal-active-io-${active_io_case}-${active_io_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
