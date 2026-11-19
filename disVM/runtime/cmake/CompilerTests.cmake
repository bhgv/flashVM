# Copyright (c) 2026 bhgv
# EN: Supported native compiler tutorials and complete desktop build workflow.
# RU: Поддерживаемые native-примеры компиляции и полный путь сборки в desktop.

if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_COMPILER)
    foreach(mode jit interpreter)
        set(dispatch --jit)
        if(mode STREQUAL "interpreter")
            set(dispatch --no-jit)
        endif()
        add_test(NAME disvm_compiler_examples_${mode} COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_compiler_examples.py"
            "$<TARGET_FILE:disvm>" "${CMAKE_CURRENT_BINARY_DIR}" "${dispatch}")
        set_tests_properties(disvm_compiler_examples_${mode} PROPERTIES TIMEOUT 150 LABELS "native;compiler")
        if(DVM_RUNTIME_EDITOR)
            add_test(NAME disvm_desktop_workflow_${mode} COMMAND "${PYTHON_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop.py" "$<TARGET_FILE:disvm-editor>"
                "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
                "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf" "${mode}")
            set_tests_properties(disvm_desktop_workflow_${mode} PROPERTIES TIMEOUT 360 LABELS "native;editor;compiler"
                ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
        endif()
    endforeach()
endif()
