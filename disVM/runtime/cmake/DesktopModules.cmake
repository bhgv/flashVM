# Copyright (c) 2026 bhgv
# EN: Build portable desktop entry modules independently of test registration.
# RU: Сборка переносимых desktop-модулей независимо от регистрации тестов.
# EN: Uses root/includes/module_headers from the runtime parent; Limbo owns UI logic.
# RU: Использует root/includes/module_headers родителя; UI-логика остаётся в Limbo.

if(DVM_RUNTIME_EDITOR)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
        "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.sbl"
        COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
            "${root}/ide/dis/app/desktop.b"
        DEPENDS limbo-bootstrap "${root}/ide/dis/app/desktop.b" ${module_headers} VERBATIM)
    add_custom_target(disvm_desktop_app ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis"
        "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.sbl"
        COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -I "${root}/ide/dis/app"
            -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis" "${root}/ide/dis/examples/desktop/main.b"
        DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop/main.b" "${root}/ide/dis/app/desktop.m" VERBATIM)
    add_custom_target(disvm_desktop_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis")
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
        -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis" "${root}/ide/dis/examples/desktop-lifecycle/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-lifecycle/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_lifecycle ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
        -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis" "${root}/ide/dis/examples/desktop-pointer/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-pointer/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_pointer_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis")

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
        -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis" "${root}/ide/dis/examples/desktop-clipboard/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-clipboard/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_clipboard_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis")

# EN: Rebuild the application and its relative imports together for installation.
# RU: Пересобирать приложение и относительные импорты вместе перед установкой.
if(DVM_RUNTIME_EDITOR)
    add_dependencies(disvm_desktop_app disvm_native_modules)
    add_dependencies(disvm-editor disvm_desktop_app disvm_desktop_example
        disvm_desktop_lifecycle disvm_desktop_pointer_example disvm_desktop_clipboard_example)
endif()
