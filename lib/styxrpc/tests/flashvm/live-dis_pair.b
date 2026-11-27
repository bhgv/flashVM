# Copyright (c) 2026 bhgv
# EN: Rebind the exported library directory while FlashVM retains its first proxy.
# RU: Перепривязать каталог библиотек, пока FlashVM удерживает первый прокси.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # System calls for this fixture.

include "named_movie_upload.b";


# EN: Wait for a marker created by the movie through ordinary VFS.
# RU: Дождаться метки, созданной фильмом через обычную VFS.
wait_for(path: string)
{
    for (i := 0; i < 800; i++) {
        (ok, nil) := sys->stat(path);
        if (ok == 0) {
            return;
        }
        sys->sleep(10);
    }
    raise "missing live module test marker " + path;
}


# EN: Replace the live union head after the first proxy has made its calls.
# RU: Заменить первый слой живого union после вызовов через первый прокси.
init()
{
    sys = load Sys Sys->PATH;
    start("live-dis.swf");
    wait_for("/loaded");
    wait_for("/direct");
    if (sys->bind("/replacement", "/library", Sys->MBEFORE) < 0) {
        raise sys->sprint("cannot replace live library directory: %r");
    }
    gate := sys->open("/gate", Sys->OWRITE);
    ready := array of byte "1";
    if (gate == nil || sys->write(gate, ready, 1) != 1) {
        raise "cannot signal live library replacement";
    }
    wait_for("/done");
    sys->print("PASS native live-dis RPC paired VMs\n");
}
