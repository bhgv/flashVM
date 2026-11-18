# Copyright (c) 2026 bhgv
# EN: Original native Styx carrier-loss tests with actual full RX, route queues and client subscription races.
# RU: Original native Styx carrier-loss tests с настоящими full RX, route queues и subscription races.
set_source_files_properties(tests/link/host.c tests/link/fixture.c tests/link/checkpoint.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/link/fixture.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(link_case RANGE 0 11)
    set(link_peer terminal-link-${link_case})
    math(EXPR link_queue "(${link_case} / 2 + 2) % 3")
    add_executable(${link_peer} tests/link/host.c tests/link/fixture.c tests/link/checkpoint.c
        tests/retirement/wire.c tests/exchange_provider.c ../tests/file-root/context.c)
    target_compile_definitions(${link_peer} PRIVATE STYX_CONTROL_LINK_TEST=1 STYX_RETIREMENT_CASE=${link_queue})
    set_property(TARGET ${link_peer} APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--wrap=pthread_cond_wait -Wl,--wrap=routeenqueue -Wl,--wrap=controlreceive -Wl,--wrap=styxclientwatch")
    target_link_libraries(${link_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    add_test(NAME ${link_peer} COMMAND ${link_peer} ${link_case})
    set_tests_properties(${link_peer} PROPERTIES TIMEOUT 15)
endforeach()
