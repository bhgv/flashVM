# Copyright (c) 2026 bhgv
# EN: Actual native provider module/dependency acceptance, independent of UI/builtin dispatch.
# RU: Настоящие native provider module/dependency tests, независимо от UI/builtin dispatch.

# EN/RU: Actual provider-backed code/dependencies run on the same dedicated VM in interpreter/JIT modes.
# Настоящие provider-backed code/dependencies выполняются в той же dedicated VM с interpreter/JIT.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_provider_modules COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/provider_modules/check.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_provider_modules PROPERTIES TIMEOUT 180 LABELS "native")
endif()
