# Copyright (c) 2026 bhgv
# EN: Explicit native per-tag ordered-packet layer over the existing standalone S-record codecs.
# RU: Явный native per-tag ordered-packet layer над existing standalone S-record codecs.
add_library(styx_terminal_order STATIC life.c receive.c inbox.c send.c rollover.c)
target_include_directories(styx_terminal_order PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_terminal_order PUBLIC styx_movie_protocol Threads::Threads)
target_compile_options(styx_terminal_order PRIVATE -Wall -Wextra -Wpedantic -Werror)

foreach(order_case codec queues rollover)
    add_executable(terminal-order-${order_case} tests/${order_case}.c)
    target_link_libraries(terminal-order-${order_case} PRIVATE styx_terminal_order)
    target_compile_options(terminal-order-${order_case} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME terminal-order-${order_case} COMMAND terminal-order-${order_case})
    set_tests_properties(terminal-order-${order_case} PROPERTIES TIMEOUT 15)
endforeach()
add_executable(terminal-order-oracle tests/oracle.c
    "${INFERNO_VFS_SUPPORT}/flashVM/src/protocol/order/prefix.c")
target_include_directories(terminal-order-oracle PRIVATE "${INFERNO_VFS_SUPPORT}/flashVM/include")
set_target_properties(terminal-order-oracle PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_compile_options(terminal-order-oracle PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME terminal-order-polynomial COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle.py" "$<TARGET_FILE:terminal-order-oracle>")

add_executable(terminal-order-wire tests/host.c tests/wire.c ../tests/link/fixture.c
    ../../tests/file-root/context.c)
set_source_files_properties(tests/wire.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(terminal-order-wire PRIVATE styx_terminal_order styx_terminal infernostyx_phy remote_phy_mux)
target_compile_options(terminal-order-wire PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unknown-pragmas)
add_test(NAME terminal-order-original-styx COMMAND terminal-order-wire)
set_tests_properties(terminal-order-original-styx PROPERTIES TIMEOUT 20)
if(INFERNO_UI_HOST_LIBRARIES)
    enable_language(CXX)
    add_executable(terminal-order-header tests/header.cc)
    set_target_properties(terminal-order-header PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    target_compile_options(terminal-order-header PRIVATE -Wall -Wextra -Werror -Wno-long-long)
    target_link_libraries(terminal-order-header PRIVATE styx_terminal_order)
    add_test(NAME terminal-order-cxx98-header COMMAND terminal-order-header)
endif()

add_executable(terminal-order-fault tests/fault.c)
target_link_libraries(terminal-order-fault PRIVATE styx_terminal_order)
target_compile_options(terminal-order-fault PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_property(TARGET terminal-order-fault APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=malloc -Wl,--wrap=calloc")
add_test(NAME terminal-order-allocation-boundary COMMAND terminal-order-fault)

add_subdirectory(broker)

add_subdirectory(files)
