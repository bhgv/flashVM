# Copyright (c) 2026 bhgv
# EN: Compile the optional C-module tutorial against its generated interface.
# RU: Сборка необязательного примера C-модуля с его сгенерированным интерфейсом.
# EN: Parent sets native_demo_root only after validating DVM_LIMBO_BINDINGS.
# RU: Родитель задаёт native_demo_root после проверки DVM_LIMBO_BINDINGS.

if(DVM_RUNTIME_NATIVE_DEMO)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis"
        "${CMAKE_CURRENT_BINARY_DIR}/native-demo.sbl"
        COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module" -I "${native_demo_root}"
            -o "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${native_demo_root}/main.b"
        DEPENDS limbo-bootstrap "${native_demo_root}/nativedemo.m"
            "${native_demo_root}/main.b" VERBATIM)
    add_custom_target(disvm_native_module_tutorial ALL
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis")
    add_dependencies(disvm disvm_native_module_tutorial)
    if(DVM_RUNTIME_EDITOR)
        add_dependencies(disvm-editor disvm_native_module_tutorial)
    endif()
endif()
