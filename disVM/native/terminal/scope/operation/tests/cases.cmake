# Copyright (c) 2026 bhgv
# EN: Real original Styx peer confirmations, uncertain outcomes and ordered native service dispatch.
# RU: Real original Styx confirmations, uncertain outcomes и ordered native service dispatch.
set_source_files_properties(operation/tests/host.c operation/tests/handler.c operation/tests/wire.c operation/tests/gate.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
foreach(operation_case RANGE 0 4)
    set(operation_peer terminal-scope-operation-${operation_case})
    add_executable(${operation_peer} operation/tests/host.c operation/tests/handler.c operation/tests/wire.c
        operation/tests/gate.c tests/wire.c ../tests/link/fixture.c ../../tests/file-root/context.c)
    target_compile_definitions(${operation_peer} PRIVATE STYX_SCOPE_OPERATION_TEST=1 STYX_SCOPE_CASE=0
        STYX_OPERATION_CASE=${operation_case})
    set_property(TARGET ${operation_peer} APPEND_STRING PROPERTY LINK_FLAGS
        " -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=styxbidircontrolsendwait")
    target_link_libraries(${operation_peer} PRIVATE styx_terminal_scope infernostyx_phy remote_phy_mux)
    add_test(NAME ${operation_peer} COMMAND ${operation_peer})
    set_tests_properties(${operation_peer} PROPERTIES TIMEOUT 30)
endforeach()
