# Copyright (c) 2026 bhgv
# EN: Generic ordinary VFS projection of already granted ordered clients, independent of VM/display.
# RU: Обычная VFS-проекция выданных ordered-клиентов, независимо от VM/display.
add_library(styx_ordered_files STATIC life.c provider.c io.c)
target_include_directories(styx_ordered_files PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_ordered_files PUBLIC styx_terminal_ordered)
target_compile_options(styx_ordered_files PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(terminal-ordered-files tests/main.c tests/cancel.c ../broker/tests/wire.c
    ../../tests/link/fixture.c ../../../tests/file-root/context.c)
target_compile_definitions(terminal-ordered-files PRIVATE ORDERED_CASE=0 ORDERED_FILE_TEST=1)
target_link_libraries(terminal-ordered-files PRIVATE styx_ordered_files styx_terminal_route infernostyx_phy remote_phy_mux)
target_compile_options(terminal-ordered-files PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unknown-pragmas)
set_property(TARGET terminal-ordered-files APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=dvm_cancel_subscribe -Wl,--wrap=calloc -Wl,--wrap=styxorderclose")
add_test(NAME terminal-ordered-files COMMAND terminal-ordered-files)
set_tests_properties(terminal-ordered-files PROPERTIES TIMEOUT 20)

# EN/RU: Optional actual guest acceptance uses explicitly selected already-built runtime/compiler, never downloads/builds one implicitly.
# Опциональная проверка настоящего guest использует явно выбранные готовые runtime/compiler, без неявной загрузки/сборки.
set(INFERNO_DISVM_RUNTIME "" CACHE FILEPATH "Explicit source-matched native DisVM console executable for guest acceptance")
set(INFERNO_LIMBO_COMPILER "" CACHE FILEPATH "Explicit source-matched native-cell Limbo compiler for guest acceptance")
if(INFERNO_DISVM_RUNTIME OR INFERNO_LIMBO_COMPILER)
    if(NOT EXISTS "${INFERNO_DISVM_RUNTIME}" OR NOT EXISTS "${INFERNO_LIMBO_COMPILER}")
        message(FATAL_ERROR "Select both existing INFERNO_DISVM_RUNTIME and INFERNO_LIMBO_COMPILER for guest acceptance")
    endif()
    add_test(NAME terminal-ordered-files-guest COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/guest.py" "${INFERNO_DISVM_RUNTIME}" "${INFERNO_LIMBO_COMPILER}"
        "${INFERNO_VFS_SUPPORT}/disVM/upstream/module" "${CMAKE_BINARY_DIR}")
    set_tests_properties(terminal-ordered-files-guest PROPERTIES TIMEOUT 180 LABELS "native-guest")
endif()

if(INFERNO_UI_HOST_LIBRARIES)
    add_executable(terminal-ordered-files-header tests/header.cc ../../../tests/file-root/context.c)
    set_target_properties(terminal-ordered-files-header PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    target_link_libraries(terminal-ordered-files-header PRIVATE styx_ordered_files)
    target_compile_options(terminal-ordered-files-header PRIVATE -Wall -Wextra -Werror -Wno-long-long)
    add_test(NAME terminal-ordered-files-cxx98 COMMAND terminal-ordered-files-header)
endif()
