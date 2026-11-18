# Copyright (c) 2026 bhgv
# EN: Explicit standalone native relay, no VM, UI-host library, SDK download or network dialing.
# RU: Явный standalone native-relay, без VM, UI-host библиотек, загрузки SDK и dial.
set(unibus_cli_sources main.c config.c connection.c session.c io.c context.c)
add_executable(unibus-bidir ${unibus_cli_sources})
target_link_libraries(unibus-bidir PRIVATE styx_terminal_ordered infernostyx_phy remote_phy_mux)
target_compile_options(unibus-bidir PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(unibus-cli-server tests/server.c context.c)
set_source_files_properties(tests/server.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(unibus-cli-server PRIVATE styx_terminal_ordered infernostyx_phy remote_phy_mux)
target_compile_options(unibus-cli-server PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unknown-pragmas)
add_test(NAME unibus-cli-executable COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli.py" "$<TARGET_FILE:unibus-bidir>" "$<TARGET_FILE:unibus-cli-server>")
set_tests_properties(unibus-cli-executable PROPERTIES TIMEOUT 60)

add_executable(unibus-cli-eof-probe ${unibus_cli_sources} tests/eof.c)
target_link_libraries(unibus-cli-eof-probe PRIVATE styx_terminal_ordered infernostyx_phy remote_phy_mux)
target_compile_options(unibus-cli-eof-probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_property(TARGET unibus-cli-eof-probe APPEND_STRING PROPERTY LINK_FLAGS
    " -Wl,--wrap=styxbidircontrolwatch -Wl,--wrap=styxbidircontrolnext -Wl,--wrap=styxorderedwait")
add_test(NAME unibus-cli-clean-eof COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli.py"
    "$<TARGET_FILE:unibus-cli-eof-probe>" "$<TARGET_FILE:unibus-cli-server>" eof-probe)
set_tests_properties(unibus-cli-clean-eof PROPERTIES TIMEOUT 20)

add_executable(unibus-cli-config tests/config.c config.c context.c)
target_link_libraries(unibus-cli-config PRIVATE styx_terminal_ordered)
target_compile_options(unibus-cli-config PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME unibus-cli-config COMMAND unibus-cli-config)

add_executable(unibus-cli-faults ${unibus_cli_sources} tests/fault.c)
target_link_libraries(unibus-cli-faults PRIVATE styx_terminal_ordered infernostyx_phy remote_phy_mux)
target_compile_options(unibus-cli-faults PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_property(TARGET unibus-cli-faults APPEND_STRING PROPERTY LINK_FLAGS
    " -Wl,--wrap=unibuscli_run -Wl,--wrap=calloc -Wl,--wrap=pthread_create -Wl,--wrap=styxorderedwatch")
add_test(NAME unibus-cli-rollback COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli.py"
    "$<TARGET_FILE:unibus-cli-faults>" "$<TARGET_FILE:unibus-cli-server>" rollback)
set_tests_properties(unibus-cli-rollback PROPERTIES TIMEOUT 45)
