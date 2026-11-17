# Copyright (c) 2026 bhgv
# EN: Carrier-neutral core and actual native Styx over nonsocket PTY acceptance.
# RU: Carrier-neutral core и настоящий native Styx через nonsocket PTY.
add_executable(phy-core-test "${CMAKE_CURRENT_SOURCE_DIR}/../../remote_phy/tests/core.c")
target_link_libraries(phy-core-test PRIVATE remote_phy)
add_test(NAME remote-phy-core COMMAND phy-core-test)
add_executable(phy-styx-client tests/phy/client.c tests/phy/limited.c)
target_link_libraries(phy-styx-client PRIVATE styx_client)
foreach(phy_case normal prefix-error reject)
    add_test(NAME remote-phy-styx-${phy_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/phy/pty.py" "$<TARGET_FILE:phy-styx-client>" "${phy_case}")
    set_tests_properties(remote-phy-styx-${phy_case} PROPERTIES TIMEOUT 20)
endforeach()
set_tests_properties(remote-phy-core PROPERTIES TIMEOUT 10)
