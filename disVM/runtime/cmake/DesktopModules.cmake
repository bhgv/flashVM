# Copyright (c) 2026 bhgv
# EN: Build portable desktop entry modules independently of test registration.
# RU: Сборка переносимых desktop-модулей независимо от регистрации тестов.
# EN: Uses root/includes/module_headers from the runtime parent; Limbo owns UI logic.
# RU: Использует root/includes/module_headers родителя; UI-логика остаётся в Limbo.

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
        "${root}/ide/dis/app/desktop.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/app/desktop.b" "${root}/ide/dis/app/desktop-ink.b"
        ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_app ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis")
add_dependencies(disvm_desktop_app disvm_native_modules)

if(DVM_TERMINAL_LAUNCH_SDK)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-paired.dis"
        COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -I "${root}/ide/dis/app"
            -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-paired.dis" "${root}/ide/dis/app/desktop-paired.b"
        DEPENDS limbo-bootstrap "${root}/ide/dis/app/desktop-paired.b" "${root}/ide/dis/app/desktop.m"
            "${u}/module/sys.m" VERBATIM)
    add_custom_target(disvm_desktop_paired ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-paired.dis")
    add_dependencies(disvm_desktop_paired disvm_desktop_app)
endif()

set(DVM_PAIRED_PLAYER_BIN "" CACHE FILEPATH "Matched SDL FlashVM StyxRPC player for the installed paired desktop")
if(DVM_PAIRED_PLAYER_BIN)
    if(NOT DVM_TERMINAL_LAUNCH_SDK OR NOT EXISTS "${DVM_PAIRED_PLAYER_BIN}" OR
        NOT DVM_AS_VIEW_COMPILER OR NOT DVM_FIR_COMPILER OR NOT DVM_SWF_LINKER)
        message(FATAL_ERROR "The paired desktop needs a built FlashVM player, terminal SDK, AS2, FIR and SWL")
    endif()
    get_filename_component(paired_fir_directory "${DVM_FIR_COMPILER}" DIRECTORY)
    file(GLOB paired_as_sources CONFIGURE_DEPENDS "${root}/ide/as/desktop-paired-*.as")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-view.swf"
        COMMAND "${PYTHON_EXECUTABLE}" "${root}/ide/build_paired_view.py" "${CMAKE_CURRENT_BINARY_DIR}/desktop-view.swf"
            --as2 "${DVM_AS_VIEW_COMPILER}" --fir "${paired_fir_directory}" --swl "${DVM_SWF_LINKER}"
        DEPENDS ${paired_as_sources} "${root}/ide/build_paired_view.py" "${DVM_AS_VIEW_COMPILER}"
            "${DVM_FIR_COMPILER}" "${DVM_FIR_OPTIMIZER}" "${DVM_SWF_LINKER}" VERBATIM)
    add_custom_target(disvm_desktop_paired_movie ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-view.swf")
    add_dependencies(disvm_desktop_paired disvm_desktop_paired_movie)
endif()

if(DVM_RUNTIME_EDITOR)
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
    add_dependencies(disvm-editor disvm_desktop_app disvm_desktop_example
        disvm_desktop_lifecycle disvm_desktop_pointer_example disvm_desktop_clipboard_example)
endif()
