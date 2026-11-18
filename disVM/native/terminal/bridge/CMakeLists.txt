# Copyright (c) 2026 bhgv
# EN: Relay adapter references the host UI API; the embedding supplies its actual implementation.
# RU: Мост использует host UI API; настоящую реализацию предоставляет embedding.
add_library(styx_terminal_bridge STATIC life.c relay.c state.c retirement.c transport.c ordered.c)
target_link_libraries(styx_terminal_bridge PUBLIC styx_terminal_route styx_terminal_ordered)
target_include_directories(styx_terminal_bridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(styx_terminal_bridge PRIVATE -Wall -Wextra -Wpedantic -Werror)

# No implicit FlashVM build/download or VM dependency in the Inferno library targets.
# Без неявной сборки/скачивания FlashVM и зависимости библиотек Inferno от VM.
set(INFERNO_UI_HOST_LIBRARIES "" CACHE STRING "Explicit real host UI and FlashVM library list for relay acceptance")
if(INFERNO_UI_HOST_LIBRARIES)
    enable_language(CXX)
    # EN/RU: FlashVM adapters use C++98; verify typed and legacy public C linkage.
    # Адаптеры FlashVM используют C++98; проверить C linkage typed и legacy API.
    add_executable(terminal-typed-header ../tests/typed_header.cc)
    target_sources(terminal-typed-header PRIVATE ../../tests/file-root/context.c)
    set_target_properties(terminal-typed-header PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    # EN/RU: Existing 9P client ABI uses long long offsets in its public headers; keep that ABI.
    # Публичные заголовки существующего 9P client используют long long offsets; ABI не менять.
    target_compile_options(terminal-typed-header PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-long-long)
    target_link_libraries(terminal-typed-header PRIVATE styx_terminal_resource)
    add_test(NAME terminal-typed-cxx98-header COMMAND terminal-typed-header)
    foreach(bridge_case RANGE 0 13)
        set(bridge_peer terminal-bridge-peer-${bridge_case})
        add_executable(${bridge_peer} ../../tests/phy-mux/peer.c ../../tests/phy/limited.c
            ../../tests/launch/remote.c ../../tests/launch/request.c ../../tests/file-root/context.c
            ../tests/server.c ../tests/bridge.c)
        target_compile_definitions(${bridge_peer} PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1
            STYX_LAUNCH_REQUEST_TEST=1 STYX_TERMINAL_RECORD_TEST=1 STYX_UI_BRIDGE_CASE=${bridge_case})
        target_include_directories(${bridge_peer} PRIVATE "${INFERNO_VFS_SUPPORT}/flashVM/ports")
        if(bridge_case EQUAL 7)
            target_sources(${bridge_peer} PRIVATE ../tests/bridge_fault.c)
            set_property(TARGET ${bridge_peer} APPEND_STRING PROPERTY LINK_FLAGS
                " -Wl,--wrap=styxbidirrouteuisend -Wl,--wrap=styxbidirroutestate")
        endif()
        if(bridge_case EQUAL 11 OR bridge_case EQUAL 13)
            target_sources(${bridge_peer} PRIVATE ../tests/bridge_retirement_fault.c)
            set_property(TARGET ${bridge_peer} APPEND_STRING PROPERTY LINK_FLAGS
                " -Wl,--wrap=styxbidirroutewatch")
        endif()
        if(bridge_case EQUAL 8)
            target_sources(${bridge_peer} PRIVATE ../tests/bridge_as.c)
            set_property(TARGET ${bridge_peer} APPEND_STRING PROPERTY LINK_FLAGS
                " -Wl,--wrap=styxbidirrouteuisend")
        endif()
        set_target_properties(${bridge_peer} PROPERTIES LINKER_LANGUAGE CXX)
        target_link_libraries(${bridge_peer} PRIVATE styx_terminal_bridge infernostyx_phy remote_phy_mux
            ${INFERNO_UI_HOST_LIBRARIES})
        foreach(bridge_transport normal cancel)
            add_test(NAME terminal-bridge-${bridge_case}-${bridge_transport} COMMAND "${PYTHON_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/phy-mux/peers.py" "$<TARGET_FILE:${bridge_peer}>" ${bridge_transport})
            set_tests_properties(terminal-bridge-${bridge_case}-${bridge_transport} PROPERTIES TIMEOUT 30)
        endforeach()
    endforeach()
endif()

include(tests/ordered/cases.cmake)
