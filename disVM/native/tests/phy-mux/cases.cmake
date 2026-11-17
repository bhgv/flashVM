# Copyright (c) 2026 bhgv
# EN: Actual simultaneous original 9P roles over a single fragmented physical carrier.
# RU: Одновременные original 9P roles через один fragmented physical carrier.
add_executable(phy-mux-peer tests/phy-mux/peer.c tests/phy/limited.c)
target_link_libraries(phy-mux-peer PRIVATE infernostyx_phy styx_client remote_phy_mux)
add_test(NAME remote-phy-mux-duplex COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/phy-mux/peers.py" "$<TARGET_FILE:phy-mux-peer>" normal)
set_tests_properties(remote-phy-mux-duplex PROPERTIES TIMEOUT 25)

add_executable(phy-mux-streams "${CMAKE_CURRENT_SOURCE_DIR}/../../remote_phy/tests/mux.c")
target_link_libraries(phy-mux-streams PRIVATE remote_phy_mux)
add_test(NAME remote-phy-mux-streams COMMAND phy-mux-streams)
set_tests_properties(remote-phy-mux-streams PROPERTIES TIMEOUT 20)

add_executable(phy-mux-failure "${CMAKE_CURRENT_SOURCE_DIR}/../../remote_phy/tests/mux-failure.c")
target_link_libraries(phy-mux-failure PRIVATE remote_phy_mux)
add_test(NAME remote-phy-mux-failure COMMAND phy-mux-failure)
set_tests_properties(remote-phy-mux-failure PROPERTIES TIMEOUT 10)

add_test(NAME remote-phy-mux-cancel COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/phy-mux/peers.py" "$<TARGET_FILE:phy-mux-peer>" cancel)
set_tests_properties(remote-phy-mux-cancel PROPERTIES TIMEOUT 25)
