# Copyright (c) 2026 bhgv
# EN: Actual captured native roots, foreign clients, local ceilings and server rwx govern channel startup.
# RU: Настоящие captured native roots, foreign clients, local ceilings и server rwx управляют channel startup.
set_source_files_properties(tests/grant/host.c tests/grant/server.c tests/grant/foreign.c tests/grant/refusal.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_source_files_properties(tests/grant/resource.c tests/grant/resource_files.c tests/grant/resource_host.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/grant/resource_files.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
set_property(SOURCE tests/grant/server.c tests/grant/foreign.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(grant_case RANGE 0 19)
    set(grant_peer terminal-grant-peer-${grant_case})
    add_executable(${grant_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c ../tests/launch/remote.c
        ../tests/launch/request.c ../tests/file-root/context.c tests/grant/host.c tests/grant/server.c
        tests/grant/foreign.c tests/grant/refusal.c)
    target_compile_definitions(${grant_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_TERMINAL_GRANT_TEST=1 STYX_GRANT_CASE=${grant_case})
    set_property(TARGET ${grant_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwalkfromcancel")
    if(grant_case GREATER 17)
        target_sources(${grant_peer} PRIVATE tests/grant/resource.c tests/grant/resource_files.c
            tests/grant/resource_host.c tests/offers_files.c tests/exchange_provider.c)
        target_compile_definitions(${grant_peer} PRIVATE STYX_GRANT_RESOURCE_TEST=1 STYX_TYPED_OFFER_TEST=1)
        target_link_libraries(${grant_peer} PRIVATE styx_terminal_resource)
    endif()
    target_link_libraries(${grant_peer} PRIVATE styx_terminal infernostyx_phy remote_phy_mux)
    foreach(grant_transport normal cancel)
        add_test(NAME terminal-grant-${grant_case}-${grant_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${grant_peer}>" ${grant_transport})
        set_tests_properties(terminal-grant-${grant_case}-${grant_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
