# Copyright (c) 2026 bhgv
# EN: Submit a mounted program through its ordinary bound VFS name.
# RU: Отправить запуск смонтированной программы по обычному bind-имени VFS.
implement Client;
include "sys.m";
include "vfsnative.m";

Client: module { init: fn(); };


# EN: Reject an unadvertised file, then submit exact binary bytes through an alias.
# RU: Проверить отказ для другого файла, затем отправить байты через псевдоним.
init()
{
    sys := load Sys Sys->PATH;
    vfs := load Vfsnative Vfsnative->PATH;
    if (vfs == nil || vfs->run("/peer/other.dis") != 0) {
        raise "unadvertised launch route accepted";
    }
    if (sys->bind("/peer/implementation.dis", "/alias.dis", Sys->MREPL) < 0) {
        raise sys->sprint("launch route setup failed: %r");
    }
    arguments := array[] of {byte 'a', byte 0, byte 'b'};
    if (vfs->runargs("/alias.dis", arguments) != 1) {
        raise "provider did not accept launch";
    }
    sys->print("PASS mounted Styx launch submission\n");
}
