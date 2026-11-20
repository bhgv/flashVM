# Copyright (c) 2026 bhgv
# EN: Verify owner-side execution using a file in the owner's working directory.
# RU: Проверка исполнения у владельца по файлу в его рабочем каталоге.
implement Owner;
include "sys.m";
include "launchcontext.m";
Owner: module { init: fn(); };


# EN/RU: Read the owner's file and report successful initialization.
# Прочитать файл владельца и подтвердить успешный запуск.
init()
{
    sys := load Sys Sys->PATH;
    fd := sys->open("owner-only.txt", Sys->OREAD);
    bytes := array[5] of byte;
    if (fd == nil || sys->read(fd, bytes, len bytes) != len bytes ||
        string bytes != "owner") {
        raise "missing owner namespace";
    }
    context := load Launchcontext Launchcontext->PATH;
    if (context == nil) {
        raise "missing owner startup context";
    }
    args := context->arguments();
    if (len args == 65536) {
        for (i := 0; i < len args; i++) {
            if (args[i] != byte (i & 255)) {
                raise "corrupted maximum-size startup payload";
            }
        }
    } else if (len args != 3 || args[0] != byte 'x' || args[1] != byte 0 || args[2] != byte 'y') {
        raise "missing owner startup payload";
    }
    sys->print("PASS owner Dis init\n");
}
