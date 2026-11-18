# Copyright (c) 2026 bhgv
# EN: Actual native accepted/ordinary launch data and console-only guests.
# RU: Настоящие native accepted/ordinary launch data и console-only guests.
# EN/RU: Native launch metadata and console FDs remain independent of FlashVM presence.
# Native launch metadata и console FDs не зависят от наличия FlashVM.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_launch_context COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/launch_context/check.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_launch_context PROPERTIES TIMEOUT 180 LABELS "native")
endif()
