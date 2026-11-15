# Copyright (c) 2026 bhgv
# EN: Generate standalone scalar VFS builtin and compile its native implementation.
# RU: Генерировать standalone scalar VFS builtin и компилировать native implementation.
add_custom_command(OUTPUT "${view}/vfsnative_decl.h" "${view}/vfsnative_table.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${view}"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_proxyhost.py"
        --module Vfsnative --compiler "$<TARGET_FILE:limbo-bootstrap>"
        --interface "${root}/disVM/module/vfsnative.m" --output "${view}"
    DEPENDS limbo-bootstrap prepare_proxyhost.py "${root}/disVM/module/vfsnative.m" VERBATIM)
add_custom_target(disvm_vfsnative_sources DEPENDS "${view}/vfsnative_decl.h" "${view}/vfsnative_table.h")
add_dependencies(disvm disvm_vfsnative_sources)
target_sources(disvm PRIVATE vfs/vfsnative.c vfs/stack.c)
list(APPEND module_names vfsctl)
list(APPEND module_sources "${root}/disVM/limbo/vfsctl.b")

# EN/RU: Native embedding SDK; separate from standard Styx/Sys interfaces.
# Native embedding SDK, отдельно от standard Styx/Sys interfaces.
target_sources(disvm PRIVATE vfs/view.c vfs/view_io.c vfs/view_list.c vfs/export_native.c vfs/publication.c)
