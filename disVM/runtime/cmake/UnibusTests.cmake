# Copyright (c) 2026 bhgv
# EN: Existing Unibus ABI on real native interpreter/JIT, without a graphical test replacement.
# RU: Настоящий native interpreter/JIT для прежнего Unibus ABI, без подмены графического runtime.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_native_unibus_launch COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_unibus_binding.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_native_unibus_launch PROPERTIES TIMEOUT 180 LABELS "native")
endif()
