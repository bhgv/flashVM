# Copyright (c) 2026 bhgv
# EN: Active native sink and original deferred Tread cancellation, independent callers and scope setup rollback.
# RU: Отмена native sink и original deferred Tread, независимые callers и rollback scope setup.
set_source_files_properties(tests/fetch_cancel/announced.c tests/fetch_cancel/typed.c tests/fetch_cancel/checkpoint.c
    tests/fetch_cancel/files.c tests/fetch_cancel/wire.c tests/fetch_cancel/typed_wire.c tests/fetch_cancel/fault.c PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/fetch_cancel/files.c tests/fetch_cancel/wire.c tests/fetch_cancel/typed_wire.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(fetch_case RANGE 0 16)
    set(fetch_peer terminal-fetch-cancel-peer-${fetch_case})
    add_executable(${fetch_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c ../tests/launch/remote.c
        ../tests/launch/request.c ../tests/file-root/context.c tests/fetch_cancel/announced.c
        tests/fetch_cancel/files.c tests/fetch_cancel/checkpoint.c tests/fetch_cancel/fault.c
        tests/fetch_cancel/wire.c tests/offers_backend.c tests/exchange_provider.c tests/server_fault.c)
    target_compile_definitions(${fetch_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_TYPED_OFFER_TEST=1 STYX_FETCH_CANCEL_CASE=${fetch_case})
    if(fetch_case EQUAL 11)
        target_compile_definitions(${fetch_peer} PRIVATE STYX_OFFER_CASE=11)
    endif()
    set_property(TARGET ${fetch_peer} APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--wrap=pthread_cond_timedwait -Wl,--wrap=dvm_cancel_subscribe -Wl,--wrap=styxbidirroutewatch")
    if(fetch_case EQUAL 4 OR fetch_case EQUAL 10)
        target_compile_definitions(${fetch_peer} PRIVATE STYX_SERVER_CASE=7)
        set_property(TARGET ${fetch_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwritecancel")
    endif()
    target_link_libraries(${fetch_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    foreach(fetch_transport normal cancel)
        add_test(NAME terminal-fetch-cancel-${fetch_case}-${fetch_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${fetch_peer}>" ${fetch_transport})
        set_tests_properties(terminal-fetch-cancel-${fetch_case}-${fetch_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
foreach(typed_fetch_case RANGE 0 4)
    set(typed_fetch_peer terminal-typed-fetch-cancel-peer-${typed_fetch_case})
    add_executable(${typed_fetch_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c ../tests/launch/remote.c
        ../tests/launch/request.c ../tests/file-root/context.c tests/fetch_cancel/typed.c
        tests/fetch_cancel/checkpoint.c tests/fetch_cancel/fault.c tests/fetch_cancel/typed_wire.c
        tests/exchange_provider.c tests/server_fault.c)
    target_compile_definitions(${typed_fetch_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_TYPED_FETCH_CASE=${typed_fetch_case})
    set_property(TARGET ${typed_fetch_peer} APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--wrap=pthread_cond_timedwait -Wl,--wrap=dvm_cancel_subscribe -Wl,--wrap=styxbidirroutewatch")
    if(typed_fetch_case EQUAL 1)
        target_compile_definitions(${typed_fetch_peer} PRIVATE STYX_SERVER_CASE=7)
        set_property(TARGET ${typed_fetch_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwritecancel")
    endif()
    target_link_libraries(${typed_fetch_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    foreach(typed_fetch_transport normal cancel)
        add_test(NAME terminal-typed-fetch-cancel-${typed_fetch_case}-${typed_fetch_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${typed_fetch_peer}>" ${typed_fetch_transport})
        set_tests_properties(terminal-typed-fetch-cancel-${typed_fetch_case}-${typed_fetch_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
