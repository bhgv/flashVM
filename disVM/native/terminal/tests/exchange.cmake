# Copyright (c) 2026 bhgv
# EN: Actual VFS resource exchange beside independent UI/general records on original fragmented 9P.
# RU: Настоящий VFS resource exchange рядом с UI/general по исходному fragmented 9P.
set_source_files_properties(tests/exchange_client.c tests/exchange_provider.c tests/exchange_server.c PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/exchange_server.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(exchange_case RANGE 0 10)
    set(exchange_peer terminal-exchange-peer-${exchange_case})
    add_executable(${exchange_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/exchange_server.c tests/exchange_provider.c tests/exchange_client.c)
    target_compile_definitions(${exchange_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_EXCHANGE_CASE=${exchange_case})
    target_link_libraries(${exchange_peer} PRIVATE styx_terminal_resource styx_movie_service infernostyx_phy remote_phy_mux)
    if(exchange_case EQUAL 9)
        set_property(TARGET ${exchange_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxbidirroutesend")
    endif()
    foreach(exchange_transport normal cancel)
        add_test(NAME terminal-exchange-${exchange_case}-${exchange_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${exchange_peer}>" ${exchange_transport})
        set_tests_properties(terminal-exchange-${exchange_case}-${exchange_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
