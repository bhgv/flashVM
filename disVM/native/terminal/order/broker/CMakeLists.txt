# Copyright (c) 2026 bhgv
# EN: Explicit native bidirectional broker with named negotiation and retained packet queues.
# RU: Явный native-брокер с именованным согласованием и очередями сохранённых пакетов.
add_library(styx_terminal_ordered STATIC life.c message.c profile.c renewal.c worker.c client.c wait.c)
target_include_directories(styx_terminal_ordered PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_terminal_ordered PUBLIC styx_terminal_order styx_terminal)
target_compile_options(styx_terminal_ordered PRIVATE -Wall -Wextra -Wpedantic -Werror)

foreach(ordered_case RANGE 0 5)
    set(ordered_peer terminal-ordered-${ordered_case})
    add_executable(${ordered_peer} tests/host.c tests/wire.c ../../tests/link/fixture.c
        ../../../tests/file-root/context.c)
    target_compile_definitions(${ordered_peer} PRIVATE ORDERED_CASE=${ordered_case})
    set_source_files_properties(tests/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
    target_compile_options(${ordered_peer} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unknown-pragmas)
    target_link_libraries(${ordered_peer} PRIVATE styx_terminal_ordered infernostyx_phy remote_phy_mux)
    add_test(NAME ${ordered_peer} COMMAND ${ordered_peer})
    set_tests_properties(${ordered_peer} PROPERTIES TIMEOUT 20)
endforeach()

add_executable(terminal-ordered-codec tests/codec.c)
target_link_libraries(terminal-ordered-codec PRIVATE styx_movie_protocol)
set_target_properties(terminal-ordered-codec PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_compile_options(terminal-ordered-codec PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME terminal-ordered-codec COMMAND terminal-ordered-codec)

add_executable(terminal-ordered-coordinator tests/coordinator.c ../../../tests/file-root/context.c)
target_link_libraries(terminal-ordered-coordinator PRIVATE styx_terminal_ordered)
target_compile_options(terminal-ordered-coordinator PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_property(TARGET terminal-ordered-coordinator APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=malloc")
add_test(NAME terminal-ordered-coordinator COMMAND terminal-ordered-coordinator)
if(INFERNO_UI_HOST_LIBRARIES)
    enable_language(CXX)
    add_executable(terminal-ordered-header tests/header.cc ../../../tests/file-root/context.c)
    set_target_properties(terminal-ordered-header PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    target_compile_options(terminal-ordered-header PRIVATE -Wall -Wextra -Werror -Wno-long-long)
    target_link_libraries(terminal-ordered-header PRIVATE styx_terminal_ordered)
    add_test(NAME terminal-ordered-cxx98-header COMMAND terminal-ordered-header)
endif()

add_executable(terminal-ordered-oracle tests/oracle.c)
target_link_libraries(terminal-ordered-oracle PRIVATE styx_movie_protocol)
set_target_properties(terminal-ordered-oracle PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_compile_options(terminal-ordered-oracle PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME terminal-ordered-wire-oracle COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle.py" "$<TARGET_FILE:terminal-ordered-oracle>")
