# Copyright (c) 2026 bhgv
# EN: Run the optional C-module tutorial with concurrent GC and permanent owners.
# RU: Проверять пример C-модуля с конкурентным GC и закреплёнными владельцами.

if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_NATIVE_DEMO)
    foreach(mode jit interpreter)
        add_test(NAME disvm_native_module_${mode} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_native_module.py" "$<TARGET_FILE:disvm>"
            "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${mode}")
        set_tests_properties(disvm_native_module_${mode} PROPERTIES TIMEOUT 90 LABELS "native;modules")
    endforeach()
endif()
