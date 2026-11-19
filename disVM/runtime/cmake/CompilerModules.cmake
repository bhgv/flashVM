# Copyright (c) 2026 bhgv
# EN: Build deployable compiler-service examples without archived test targets.
# RU: Сборка поставляемых примеров сервиса компиляции без архивных тестовых целей.
# EN: Inherits root/includes/module_headers; no compiler CLI runs inside the VM.
# RU: Использует root/includes/module_headers; внутри VM не запускается CLI-компилятор.

if(DVM_RUNTIME_COMPILER)
    set(compiler_examples)
    foreach(example compiler-service build-panel artifact-export)
        set(source "${root}/ide/dis/examples/${example}/main.b")
        set(product "${CMAKE_CURRENT_BINARY_DIR}/${example}.dis")
        add_custom_command(OUTPUT "${product}" "${CMAKE_CURRENT_BINARY_DIR}/${example}.sbl"
            COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${product}" "${source}"
            DEPENDS limbo-bootstrap "${source}" ${module_headers} VERBATIM)
        add_custom_target(disvm_${example} ALL DEPENDS "${product}")
        list(APPEND compiler_examples "${product}")
    endforeach()
    add_custom_target(disvm_compiler_examples ALL DEPENDS ${compiler_examples})
    add_dependencies(disvm_compiler_examples disvm_native_modules)
    add_dependencies(disvm disvm_compiler_examples)
    if(DVM_RUNTIME_EDITOR)
        add_dependencies(disvm-editor disvm_compiler_examples)
    endif()
endif()
