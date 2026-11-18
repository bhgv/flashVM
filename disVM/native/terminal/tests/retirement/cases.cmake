# Copyright (c) 2026 bhgv
# EN: Real original Styx failure while each bounded dispatcher queue is full, plus rollback.
# RU: Real original Styx failure при заполнении каждой bounded dispatcher queue, плюс rollback.
set_source_files_properties(tests/retirement/host.c tests/retirement/wire.c tests/retirement/fault.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/retirement/wire.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(retirement_case RANGE 0 7)
    set(retirement_peer terminal-retirement-peer-${retirement_case})
    add_executable(${retirement_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/retirement/host.c tests/retirement/wire.c tests/retirement/fault.c
        tests/exchange_provider.c tests/server_fault.c)
    target_compile_definitions(${retirement_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_SERVER_CASE=7
        STYX_RETIREMENT_CASE=${retirement_case})
    set_property(TARGET ${retirement_peer} APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--wrap=pthread_cond_wait -Wl,--wrap=styxclientwritecancel -Wl,--wrap=styxbidircontrolwatch")
    target_link_libraries(${retirement_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    foreach(retirement_transport normal cancel)
        add_test(NAME terminal-retirement-${retirement_case}-${retirement_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${retirement_peer}>" ${retirement_transport})
        set_tests_properties(terminal-retirement-${retirement_case}-${retirement_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
