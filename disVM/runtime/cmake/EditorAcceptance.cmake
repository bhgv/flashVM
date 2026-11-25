# Copyright (c) 2026 bhgv
# EN: Native editor deployment/idle acceptance, registered after archived probes.
# RU: Приёмка установки/простоя editor после архивных проверок.

# EN: Check the portable guest and relative imports through the shipped launcher.
# RU: Проверять переносимого гостя и относительные импорты через штатный launcher.
if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_EDITOR AND UNIX)
    add_test(NAME disvm_desktop_installed COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_install.py"
        "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/run-desktop-local.sh")
    set_tests_properties(disvm_desktop_installed PROPERTIES TIMEOUT 200 LABELS "native;editor")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME disvm_editor_idle_cpu COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_editor_idle.py" "$<TARGET_FILE:disvm-editor>"
            "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
            "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf")
        set_tests_properties(disvm_editor_idle_cpu PROPERTIES TIMEOUT 90 LABELS "native;editor")
    endif()
endif()

# EN/RU: These SDL tests use the real queue; dummy video is explicit, not hardware.
# Настоящая очередь SDL с явным dummy video; это не физическое оборудование.
if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_EDITOR AND DVM_EDITOR_SDL)
    foreach(test disvm_sdl_window_lifecycle disvm_sdl_text_bursts disvm_paint_input_queue)
        set_tests_properties(${test} PROPERTIES DISABLED FALSE LABELS "native;editor;sdl"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    endforeach()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME disvm_sdl_idle_window COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_sdl_idle.py" "$<TARGET_FILE:disvm-editor>"
            "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
            "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf")
        set_tests_properties(disvm_sdl_idle_window PROPERTIES TIMEOUT 90 LABELS "native;editor;sdl")
    endif()
endif()
