# Copyright (c) 2026 bhgv
# EN: Native client adapter uses original lib9 codecs, no second wire implementation.
# RU: Native client adapter использует lib9 codecs без второго wire implementation.
if(NOT TARGET remote_phy)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../remote_phy" "${CMAKE_CURRENT_BINARY_DIR}/remote_phy")
endif()
add_library(styx_client STATIC session.c fid.c clunk.c walk.c open.c operation.c forget.c io.c stat.c fd.c phy.c cancel.c mux.c mux_loop.c heartbeat.c life.c)
target_include_directories(styx_client PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../vfs")
target_link_libraries(styx_client PUBLIC inferno9 Threads::Threads remote_phy)
target_compile_options(styx_client PRIVATE -Wall -Wextra -fno-strict-aliasing)
