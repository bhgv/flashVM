# Copyright (c) 2026 bhgv
# EN: Successful guest chunks carry change hints without diagnostic stat or retry.
# RU: Успешные guest chunks несут change hints без diagnostic stat и retry.
implement EventCheck;
include "sys.m";
EventCheck: module { init: fn(); };


# EN/RU: Read exactly three provider chunks, retaining ordinary FD semantics.
# Прочитать ровно три provider chunks, сохранив ordinary FD semantics.
init()
{
    sys := load Sys Sys->PATH;
    fd := sys->open("view/stream", Sys->OREAD);
    if (fd == nil) {
        raise "event provider open";
    }
    bytes := array[1] of byte;
    for (i := 0; i < 3; i++) {
        if (sys->read(fd, bytes, 1) != 1 || int bytes[0] != int 'A' + i) {
            raise "event hint changed data/count or retried I/O";
        }
    }
    if (sys->unmount(nil, "view") != 0) {
        raise "event provider unmount";
    }
    sys->print("PASS provider event guest preserves ABC chunks\n");
}
