# Copyright (c) 2026 bhgv
# EN: Stage an AS caller and observe its file and child in the live Dis namespace.
# RU: Загрузить AS и наблюдать файл и дочерний клип в живом пространстве Dis.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys;


# EN: Upload the compiled test movie in confirmed chunks and start its owner.
# RU: Загрузить собранный фильм подтверждёнными порциями и запустить.
start()
{
    source := sys->open("os-client.swf", Sys->OREAD);
    bytes := array[32768] of byte;
    size := sys->read(source, bytes[4:], len bytes - 4);
    fd := sys->open("/rpc/.upload", Sys->OWRITE);
    if (size <= 0 || size >= len bytes - 4 || fd == nil) {
        raise sys->sprint("cannot stage os movie: %r");
    }
    bytes[2] = byte (size >> 8);
    bytes[3] = byte size;
    for (offset := 0; offset < size + 4;) {
        sent := sys->write(fd, bytes[offset:size + 4], size + 4 - offset);
        if (sent <= 0) {
            raise sys->sprint("upload os movie: %r");
        }
        offset += sent;
    }
    fd = sys->open("/rpc/.state", Sys->OWRITE);
    command := array of byte "start";
    if (fd == nil || sys->write(fd, command, len command) != len command) {
        raise sys->sprint("start os movie: %r");
    }
}


# EN: Fetch the current first byte of path, returning -1 on failure.
# RU: Прочитать текущий первый байт path либо вернуть -1 при ошибке.
marker(path: string): int
{
    file := sys->open(path, Sys->OREAD);
    data := array[1] of byte;
    if (file == nil || sys->read(file, data, 1) != 1) {
        return -1;
    }
    return int data[0];
}


# EN: Wait for both writes originating in the FlashVM context and its loaded child.
# RU: Дождаться записи из контекста FlashVM и его загруженного потомка.
init()
{
    sys = load Sys Sys->PATH;
    start();
    for (attempt := 0; attempt < 100; ++attempt) {
        if (marker("os-assets/marker.txt") == 90 && marker("child.txt") == 67) {
            sys->print("PASS os VFS handles and attached SWF\n");
            return;
        }
        sys->sleep(20);
    }
    raise sys->sprint("os markers differ: %d %d", marker("os-assets/marker.txt"), marker("child.txt"));
}
