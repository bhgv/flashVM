# Copyright (c) 2026 bhgv
# EN: Compose deployable runtime products independently of acceptance registration.
# RU: Сборка поставляемых runtime-продуктов независимо от регистрации проверок.
# EN: Parent owns root/includes/module_headers and disvm; editor remains optional.
# RU: Родитель задаёт root/includes/module_headers и disvm; editor необязателен.

include("${CMAKE_CURRENT_LIST_DIR}/EditorTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/DesktopModules.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CompilerModules.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NativeModules.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/RuntimeExamples.cmake")

# EN: The installer requires these structural tools even in non-graphical packages.
# They inspect Dis64 containers; they do not execute Dis64 code.
# RU: Установщик требует структурные инструменты и в неграфических пакетах.
# Они проверяют контейнеры Dis64, но не исполняют код Dis64.
add_subdirectory("${root}/disVM/envelope" envelope)
