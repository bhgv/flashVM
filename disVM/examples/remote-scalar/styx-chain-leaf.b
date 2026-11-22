# Copyright (c) 2026 bhgv
# EN: Execute the final image on C using local code and the inherited A context.
# RU: Выполнить конечный образ на C с локальным кодом и унаследованным контекстом A.
implement Chainleaf;
include "sys.m";
include "launchcontext.m";

Chainleaf: module { init: fn(); };


# EN: Check C's local VFS, A's live VFS and the inherited numbered descriptors.
# RU: Проверить локальную VFS C, живую VFS A и унаследованные номерные дескрипторы.
init()
{
    sys := load Sys Sys->PATH;
    context := load Launchcontext Launchcontext->PATH;
    action := string context->arguments();
    if (action == "park") {
        park_nested(sys);
        return;
    }
    if (action == "fail") {
        raise "requested nested launch failure";
    }
    if (action != "leaf") {
        raise "unexpected nested launch arguments";
    }
    local := sys->open("/leaf-owner", Sys->OREAD);
    marker := sys->open("/.vfs/chain-stage", Sys->OREAD);
    restricted := sys->fildes(3);
    if (local == nil || marker == nil || restricted == nil ||
        sys->open("/.prog/.self/.fd/3", Sys->OWRITE) != nil) {
        raise "nested launch lost local or inherited context";
    }
    data := array[4] of byte;
    if (sys->readn(local, data, 4) != 4 || string data != "on C" ||
        sys->readn(marker, data, 4) != 4 || data[0] != byte 0 || data[1] != byte 0 ||
        data[2] != byte 0 || data[3] != byte 42) {
        raise "nested launch did not distinguish its local and inherited VFS";
    }
    a := sys->fildes(7);
    b := sys->open("/.prog/.self/.fd/8", Sys->ORDWR);
    evidence := array[] of {byte 'E'};
    signal := array[] of {byte 'F'};
    if (a == nil || b == nil || sys->write(a, evidence, 1) != 1 || sys->write(b, signal, 1) != 1 ||
        sys->fprint(sys->fildes(1), "chain leaf stdout\n") <= 0 ||
        sys->fprint(sys->fildes(2), "chain leaf stderr\n") <= 0) {
        raise "nested launch lost the inherited output descriptors";
    }
}


# EN: Signal A through its inherited VFS, then stay in a pure big instruction loop.
# RU: Отметить запуск через VFS A и остаться в чистом цикле инструкций big.
park_nested(sys: Sys)
{
    fd := sys->open("/.vfs/chain-parked", Sys->OWRITE);
    notice := array[] of {byte 0, byte 0, byte 0, byte 1};
    if (fd == nil || sys->write(fd, notice, len notice) != len notice) {
        raise "could not report nested cancellation checkpoint";
    }
    remaining := big 1 << 50;
    while (remaining != big 0) {
        remaining--;
    }
    raise "nested launch was not cancelled";
}
