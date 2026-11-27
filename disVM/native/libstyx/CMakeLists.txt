# Copyright (c) 2026 bhgv
# EN: Original C server and its Linux POSIX transport, linked against complete native lib9.
# RU: Исходный C-сервер и транспорт Linux POSIX, связанные с полной native lib9.
add_library(infernostyx STATIC "${INFERNO_SOURCE}/tools/libstyx/styxserver.c" "${INFERNO_SOURCE}/tools/libstyx/Posix.c"
    hosted_decode.c)
set_property(SOURCE "${INFERNO_SOURCE}/tools/libstyx/styxserver.c" APPEND PROPERTY
    COMPILE_DEFINITIONS convM2S=styx_hosted_request_decode)
target_include_directories(infernostyx PUBLIC "${INFERNO_SOURCE}/include" PRIVATE "${INFERNO_SOURCE}/tools/libstyx")
target_link_libraries(infernostyx PUBLIC inferno9)
target_compile_options(infernostyx PRIVATE -Wall -Wextra -fno-strict-aliasing)
set_target_properties(infernostyx PROPERTIES OUTPUT_NAME styx)
