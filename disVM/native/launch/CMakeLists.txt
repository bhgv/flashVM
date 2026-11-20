# Copyright (c) 2026 bhgv
# EN: Native pre-init root adapter and actual ordinary/bidir Styx launch-environment acceptance.
# RU: Native pre-init root adapter и actual ordinary/bidir Styx launch-environment acceptance.
add_library(styx_launch STATIC prepare.c request.c program_record.c)
target_include_directories(styx_launch PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(styx_launch PUBLIC styx_namespace)
target_compile_options(styx_launch PRIVATE -Wall -Wextra)

foreach(launch_transport phy bidir)
    if(launch_transport STREQUAL "phy")
        set(launch_peer ../tests/phy-server/client.c)
    else()
        set(launch_peer ../tests/phy-mux/peer.c)
    endif()
    add_executable(launch-${launch_transport}-peer ${launch_peer} ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/file-root/context.c)
    target_compile_definitions(launch-${launch_transport}-peer PRIVATE STYX_LAUNCH_TEST=1)
    target_link_libraries(launch-${launch_transport}-peer PRIVATE styx_launch infernostyx_phy remote_phy_mux)
endforeach()
add_test(NAME vfs-launch-phy COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-server/peers.py" "$<TARGET_FILE:phy-server-test>"
    "$<TARGET_FILE:launch-phy-peer>" normal)
add_test(NAME vfs-launch-bidir COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:launch-bidir-peer>" normal)
add_test(NAME vfs-launch-bidir-cancel COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:launch-bidir-peer>" cancel)
set_tests_properties(vfs-launch-phy vfs-launch-bidir vfs-launch-bidir-cancel PROPERTIES TIMEOUT 30)

# EN/RU: Explicit Inferno error vocabulary enables only acknowledged missing-name fallback.
# Явный Inferno vocabulary включает только подтверждённый missing-name fallback.
foreach(lookup_transport phy bidir)
    if(lookup_transport STREQUAL "phy")
        set(lookup_peer ../tests/phy-server/client.c)
    else()
        set(lookup_peer ../tests/phy-mux/peer.c)
    endif()
    add_executable(launch-lookup-${lookup_transport}-peer ${lookup_peer} ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/file-root/context.c)
    target_compile_definitions(launch-lookup-${lookup_transport}-peer PRIVATE STYX_LAUNCH_TEST=1 STYX_INFERNO_LOOKUP_TEST=1)
    target_link_libraries(launch-lookup-${lookup_transport}-peer PRIVATE styx_launch infernostyx_phy remote_phy_mux)
endforeach()
add_test(NAME vfs-launch-lookup-phy COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-server/peers.py" "$<TARGET_FILE:phy-server-test>"
    "$<TARGET_FILE:launch-lookup-phy-peer>" normal)
foreach(lookup_case normal cancel)
    add_test(NAME vfs-launch-lookup-bidir-${lookup_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:launch-lookup-bidir-peer>" ${lookup_case})
    set_tests_properties(vfs-launch-lookup-bidir-${lookup_case} PROPERTIES TIMEOUT 30)
endforeach()
set_tests_properties(vfs-launch-lookup-phy PROPERTIES TIMEOUT 30)

# EN/RU: Actual attached terminal root through native launch tickets and one-use handoff.
# Реальный attached terminal root через native launch tickets и one-use handoff.
foreach(request_transport phy bidir)
    if(request_transport STREQUAL "phy")
        set(request_peer ../tests/phy-server/client.c)
    else()
        set(request_peer ../tests/phy-mux/peer.c)
    endif()
    add_executable(launch-request-${request_transport}-peer ${request_peer} ../tests/phy/limited.c
        ../tests/launch/remote.c ../tests/launch/request.c ../tests/file-root/context.c)
    target_compile_definitions(launch-request-${request_transport}-peer PRIVATE STYX_LAUNCH_TEST=1
        STYX_INFERNO_LOOKUP_TEST=1 STYX_LAUNCH_REQUEST_TEST=1)
    target_link_libraries(launch-request-${request_transport}-peer PRIVATE styx_launch infernostyx_phy remote_phy_mux)
endforeach()
add_test(NAME vfs-launch-request-phy COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-server/peers.py" "$<TARGET_FILE:phy-server-test>"
    "$<TARGET_FILE:launch-request-phy-peer>" normal)
foreach(request_case normal cancel)
    add_test(NAME vfs-launch-request-bidir-${request_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tests/phy-mux/peers.py" "$<TARGET_FILE:launch-request-bidir-peer>" ${request_case})
    set_tests_properties(vfs-launch-request-bidir-${request_case} PROPERTIES TIMEOUT 30)
endforeach()
set_tests_properties(vfs-launch-request-phy PROPERTIES TIMEOUT 30)

add_executable(launch-request-fault ../tests/launch/request_fault.c ../tests/file-root/context.c)
target_link_libraries(launch-request-fault PRIVATE styx_launch)
foreach(request_fault file staterror secondstat loss badtag cancel cancelcheck)
    add_test(NAME vfs-launch-request-${request_fault} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tests/launch/request_fault.py" "$<TARGET_FILE:launch-request-fault>" ${request_fault})
    set_tests_properties(vfs-launch-request-${request_fault} PROPERTIES TIMEOUT 20)
endforeach()
