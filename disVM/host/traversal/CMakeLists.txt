# Copyright (c) 2026 bhgv
# EN: Shared native VFS traversal history, no dependency on VM heaps or a physical carrier.
# RU: Общая native history обхода VFS, без зависимости от VM heaps или physical carrier.
add_library(vfs_walk STATIC walk.c wire.c packet.c "${CMAKE_CURRENT_SOURCE_DIR}/../../../flashVM/src/protocol/srecord.c")
target_include_directories(vfs_walk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../../flashVM/include")
target_link_libraries(vfs_walk PUBLIC Threads::Threads)
set_target_properties(vfs_walk PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)
