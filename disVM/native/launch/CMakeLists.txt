# Copyright (c) 2026 bhgv
# EN: Native pre-init root adapter and actual ordinary/bidir Styx launch-environment acceptance.
# RU: Native pre-init root adapter и actual ordinary/bidir Styx launch-environment acceptance.
add_library(styx_launch STATIC prepare.c)
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
