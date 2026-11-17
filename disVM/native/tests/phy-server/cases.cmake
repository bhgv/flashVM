# Copyright (c) 2026 bhgv
# EN: Actual original server/client over non-socket carriers and native deferred wake/cancel coverage.
# RU: Actual original server/client через non-socket carriers и native deferred wake/cancel coverage.
add_executable(phy-server-test tests/phy-server/server.c tests/phy/limited.c)
target_include_directories(phy-server-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/server/phy")
target_link_libraries(phy-server-test PRIVATE infernostyx_phy Threads::Threads)
add_executable(phy-server-peer tests/phy-server/client.c tests/phy/limited.c)
target_link_libraries(phy-server-peer PRIVATE styx_client)
foreach(phy_server_case normal cancel disconnect)
    add_test(NAME remote-phy-server-${phy_server_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/phy-server/peers.py" "$<TARGET_FILE:phy-server-test>"
        "$<TARGET_FILE:phy-server-peer>" "${phy_server_case}")
    set_tests_properties(remote-phy-server-${phy_server_case} PROPERTIES TIMEOUT 20)
endforeach()

foreach(phy_oracle_case normal badlen)
    add_test(NAME remote-phy-server-oracle-${phy_oracle_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/phy-server/oracle.py" "$<TARGET_FILE:phy-server-test>"
        "${INFERNO_SOURCE}/tools/libstyx/tests" "${phy_oracle_case}")
    set_tests_properties(remote-phy-server-oracle-${phy_oracle_case} PROPERTIES TIMEOUT 20)
endforeach()

add_executable(phy-server-queue tests/phy-server/queue.c)
target_link_libraries(phy-server-queue PRIVATE infernostyx_phy)
add_test(NAME remote-phy-server-queue COMMAND phy-server-queue)
set_tests_properties(remote-phy-server-queue PROPERTIES TIMEOUT 10)
