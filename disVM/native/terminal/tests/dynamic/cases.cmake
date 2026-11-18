# Copyright (c) 2026 bhgv
# EN: Real native Styx stress beyond former queue limits plus actual dispatcher allocation refusal.
# RU: Real native Styx stress сверх прежних queue limits плюс настоящий отказ allocation dispatcher.
set_source_files_properties(tests/dynamic/host.c tests/dynamic/fault.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
foreach(dynamic_case RANGE 0 5)
    set(dynamic_peer terminal-dynamic-${dynamic_case})
    math(EXPR dynamic_queue "${dynamic_case} % 3")
    add_executable(${dynamic_peer} tests/dynamic/host.c tests/dynamic/fault.c tests/link/fixture.c
        tests/retirement/wire.c ../tests/file-root/context.c)
    target_compile_definitions(${dynamic_peer} PRIVATE STYX_DYNAMIC_TEST=1 STYX_DYNAMIC_CASE=${dynamic_case}
        STYX_RETIREMENT_CASE=${dynamic_queue})
    set_property(TARGET ${dynamic_peer} APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--wrap=routeenqueue -Wl,--wrap=malloc")
    target_link_libraries(${dynamic_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    add_test(NAME ${dynamic_peer} COMMAND ${dynamic_peer})
    set_tests_properties(${dynamic_peer} PROPERTIES TIMEOUT 30)
endforeach()
