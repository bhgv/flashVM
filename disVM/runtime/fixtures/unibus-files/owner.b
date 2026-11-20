# Copyright (c) 2026 bhgv
# EN: Verify owner-side execution using a file in the owner's working directory.
# RU: Проверка исполнения у владельца по файлу в его рабочем каталоге.
implement Owner;
include "sys.m";
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
    sys->print("PASS owner Dis init\n");
}
