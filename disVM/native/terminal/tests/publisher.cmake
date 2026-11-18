# Copyright (c) 2026 bhgv
# EN: Real fragmented 9P exercises native source-to-Unibus publication and independent worker shutdown.
# RU: Real fragmented 9P проверяет native source-to-Unibus publication и независимое завершение workers.
set_source_files_properties(tests/publisher_host.c tests/publisher_wire.c tests/publisher_watch.c PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/publisher_wire.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(publisher_case RANGE 0 10)
    set(publisher_peer terminal-publisher-peer-${publisher_case})
    add_executable(${publisher_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/publisher_host.c tests/publisher_wire.c tests/publisher_watch.c tests/offers_files.c tests/offers_backend.c
        tests/exchange_provider.c tests/server_fault.c)
    target_compile_definitions(${publisher_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_TYPED_OFFER_TEST=1
        STYX_PUBLISHER_CASE=${publisher_case})
    if(publisher_case EQUAL 4 OR publisher_case EQUAL 7 OR publisher_case EQUAL 9)
        target_compile_definitions(${publisher_peer} PRIVATE STYX_SERVER_CASE=6)
        set_property(TARGET ${publisher_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwritecancel")
    elseif(publisher_case EQUAL 6)
        target_compile_definitions(${publisher_peer} PRIVATE STYX_SERVER_CASE=7)
        set_property(TARGET ${publisher_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwritecancel")
    elseif(publisher_case EQUAL 8)
        target_compile_definitions(${publisher_peer} PRIVATE STYX_SERVER_CASE=9)
        set_property(TARGET ${publisher_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=pthread_create")
    endif()
    target_link_libraries(${publisher_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    foreach(publisher_transport normal cancel)
        add_test(NAME terminal-publisher-${publisher_case}-${publisher_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${publisher_peer}>" ${publisher_transport})
        set_tests_properties(terminal-publisher-${publisher_case}-${publisher_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
