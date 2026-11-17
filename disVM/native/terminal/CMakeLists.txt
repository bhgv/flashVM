# Copyright (c) 2026 bhgv
# EN: Generic native terminal record transport reuses original FlashVM codecs, no graphics.
# RU: Generic native terminal record transport использует FlashVM codecs, без графики.
add_library(styx_terminal STATIC life.c queue.c io.c "${INFERNO_VFS_SUPPORT}/flashVM/src/protocol/sstream.c")
target_include_directories(styx_terminal PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_terminal PUBLIC styx_launch styx_movie_protocol)
target_compile_options(styx_terminal PRIVATE -Wall -Wextra)

add_executable(terminal-record-peer ../tests/phy-mux/peer.c ../tests/phy/limited.c
    ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c tests/server.c tests/client.c)
target_compile_definitions(terminal-record-peer PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
    STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1)
target_link_libraries(terminal-record-peer PRIVATE styx_terminal infernostyx_phy remote_phy_mux)
foreach(terminal_case normal cancel)
    add_test(NAME terminal-record-${terminal_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:terminal-record-peer>" ${terminal_case})
    set_tests_properties(terminal-record-${terminal_case} PROPERTIES TIMEOUT 30)
endforeach()

add_executable(terminal-record-fault tests/faults.c)
target_link_libraries(terminal-record-fault PRIVATE styx_terminal)
set_property(TARGET terminal-record-fault APPEND_STRING PROPERTY LINK_FLAGS
    " -Wl,--wrap=styxclientreadwatch -Wl,--wrap=styxclientwritecancel")
foreach(record_fault crc truncated limit batch short uncertain)
    add_test(NAME terminal-record-${record_fault} COMMAND terminal-record-fault ${record_fault})
    set_tests_properties(terminal-record-${record_fault} PROPERTIES TIMEOUT 10)
endforeach()
