# Copyright (c) 2026 bhgv
# EN: Automatic hysteretic observations on original native Styx count/byte backlogs.
# RU: Автоматические hysteretic observations на original native Styx count/byte backlogs.
set_source_files_properties(tests/pressure/observe.c PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
foreach(pressure_case RANGE 0 5)
    set(pressure_peer terminal-pressure-${pressure_case})
    math(EXPR pressure_queue "${pressure_case} % 3")
    add_executable(${pressure_peer} tests/dynamic/host.c tests/dynamic/fault.c tests/link/fixture.c
        tests/retirement/wire.c tests/pressure/observe.c ../tests/file-root/context.c)
    target_compile_definitions(${pressure_peer} PRIVATE STYX_DYNAMIC_TEST=1 STYX_DYNAMIC_CASE=${pressure_queue}
        STYX_RETIREMENT_CASE=${pressure_queue} STYX_PRESSURE_TEST=1 STYX_PRESSURE_CASE=${pressure_case})
    set_property(TARGET ${pressure_peer} APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--wrap=routeenqueue -Wl,--wrap=malloc")
    target_link_libraries(${pressure_peer} PRIVATE styx_terminal_resource infernostyx_phy remote_phy_mux)
    add_test(NAME ${pressure_peer} COMMAND ${pressure_peer})
    set_tests_properties(${pressure_peer} PROPERTIES TIMEOUT 30)
endforeach()
set_source_files_properties(tests/pressure/unit.c PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
add_executable(terminal-pressure-unit tests/pressure/unit.c ../tests/file-root/context.c)
set_property(TARGET terminal-pressure-unit APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=clock_gettime")
target_link_libraries(terminal-pressure-unit PRIVATE styx_terminal_route)
add_test(NAME terminal-pressure-unit COMMAND terminal-pressure-unit)
set_tests_properties(terminal-pressure-unit PROPERTIES TIMEOUT 10)
