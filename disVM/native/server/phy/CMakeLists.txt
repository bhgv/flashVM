# Copyright (c) 2026 bhgv
# EN: Alternative original-server backend; never link both platform backends into one program.
# RU: Альтернативный backend исходного server; не линковать два platform backend в одну программу.
add_library(infernostyx_phy STATIC core.c backend.c io.c lifetime.c)
target_include_directories(infernostyx_phy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${INFERNO_SOURCE}/include"
    PRIVATE "${INFERNO_SOURCE}/tools/libstyx")
target_link_libraries(infernostyx_phy PUBLIC inferno9 remote_phy)
target_compile_options(infernostyx_phy PRIVATE -Wall -Wextra -fno-strict-aliasing)
