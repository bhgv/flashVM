# Copyright (c) 2026 bhgv
# EN: Build the native-only startup snapshot and matched read-only builtin.
# RU: Сборка native-only startup-копии и matched read-only builtin.
# EN/RU: Read-only accepted startup data has a separate typed interface, not VFS authority.
# Read-only данные принятого запуска имеют отдельный typed interface, без VFS authority.
add_custom_command(OUTPUT "${view}/launchcontext_decl.h" "${view}/launchcontext_table.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${view}"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_proxyhost.py"
        --module Launchcontext --compiler "$<TARGET_FILE:limbo-bootstrap>"
        --interface "${root}/disVM/module/launchcontext.m" --output "${view}"
    DEPENDS limbo-bootstrap prepare_proxyhost.py "${root}/disVM/module/launchcontext.m" VERBATIM)
add_custom_target(disvm_launchcontext_sources DEPENDS "${view}/launchcontext_decl.h" "${view}/launchcontext_table.h")
add_dependencies(disvm disvm_launchcontext_sources)
target_sources(disvm PRIVATE vfs/launch/context.c vfs/launch/managed.c)
