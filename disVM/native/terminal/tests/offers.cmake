# Copyright (c) 2026 bhgv
# EN: Existing offer protocol drives independent per-open resource fids on the same real bidir 9P client.
# RU: Existing offer protocol управляет отдельными per-open resource fids того же real bidir 9P client.
set_source_files_properties(tests/offers_client.c tests/offers_wire.c tests/offers_backend.c tests/offers_files.c PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/offers_wire.c tests/offers_files.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(offer_case RANGE 0 13)
    set(offer_peer terminal-offers-peer-${offer_case})
    add_executable(${offer_peer} ../tests/phy-mux/peer.c ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c
        tests/offers_client.c tests/offers_wire.c tests/offers_files.c tests/offers_backend.c
        tests/exchange_provider.c tests/server_fault.c)
    target_compile_definitions(${offer_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
        STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_TYPED_OFFER_TEST=1 STYX_OFFER_CASE=${offer_case})
    if(offer_case EQUAL 10)
        target_compile_definitions(${offer_peer} PRIVATE STYX_SERVER_CASE=9)
        set_property(TARGET ${offer_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=pthread_create")
    endif()
    target_link_libraries(${offer_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    foreach(offer_transport normal cancel)
        add_test(NAME terminal-offers-${offer_case}-${offer_transport} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:${offer_peer}>" ${offer_transport})
        set_tests_properties(terminal-offers-${offer_case}-${offer_transport} PROPERTIES TIMEOUT 30)
    endforeach()
endforeach()
