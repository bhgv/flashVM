# Copyright (c) 2026 bhgv
# EN: Explicit nested S-record broker, native namespace authority and opaque application inbox capabilities.
# RU: Явный nested S-record broker, native namespace authority и opaque application inbox capabilities.
add_library(styx_terminal_scope STATIC life.c tree.c envelope.c run.c inbox.c send.c retirement.c)
target_include_directories(styx_terminal_scope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_terminal_scope PUBLIC styx_terminal)
target_compile_options(styx_terminal_scope PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_source_files_properties(tests/host.c tests/wire.c tests/fault.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror")
set_property(SOURCE tests/wire.c APPEND PROPERTY COMPILE_OPTIONS -Wno-unknown-pragmas)
foreach(scope_case RANGE 0 11)
    set(scope_peer terminal-scope-${scope_case})
    add_executable(${scope_peer} tests/host.c tests/wire.c tests/fault.c ../tests/link/fixture.c
        ../../tests/file-root/context.c)
    target_compile_definitions(${scope_peer} PRIVATE STYX_SCOPE_CASE=${scope_case})
    set_property(TARGET ${scope_peer} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=scopefeed -Wl,--wrap=malloc -Wl,--wrap=styxbidircontrolwatch -Wl,--wrap=pthread_create")
    target_link_libraries(${scope_peer} PRIVATE styx_terminal_scope infernostyx_phy remote_phy_mux)
    add_test(NAME ${scope_peer} COMMAND ${scope_peer})
    set_tests_properties(${scope_peer} PROPERTIES TIMEOUT 30)
endforeach()
if(INFERNO_UI_HOST_LIBRARIES)
    enable_language(CXX)
    add_executable(terminal-scope-header tests/header.cc ../../tests/file-root/context.c)
    set_target_properties(terminal-scope-header PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    target_compile_options(terminal-scope-header PRIVATE -Wall -Wextra -Werror -Wno-long-long)
    target_link_libraries(terminal-scope-header PRIVATE styx_terminal_scope)
    add_test(NAME terminal-scope-cxx98-header COMMAND terminal-scope-header)
endif()
