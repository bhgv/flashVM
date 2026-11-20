# Copyright (c) 2026 bhgv
# EN: Use typed load on a Styx module directory and on its bound alias.
# RU: Типизированная загрузка каталога модуля Styx и его bind-алиаса.
implement Client;
include "sys.m";
include "compute.m";
Client: module { init: fn(); };
Wrong: module { compute: fn(value: big): big; };


# EN/RU: Check compiled import admission, normal results and a remote exception.
# Проверить допуск импорта, обычный результат и удалённое исключение.
init()
{
    sys := load Sys Sys->PATH;
    path := "/peer/mounted/compute.dis";
    if ((load Wrong path) != nil) {
        raise "wrong import admitted";
    }
    service := load Compute path;
    if (service == nil) {
        raise sys->sprint("could not load mounted Compute: %r");
    }
    if (sys->bind(path, "/alias.dis", Sys->MREPL) < 0) {
        raise sys->sprint("could not bind mounted Compute: %r");
    }
    alias := load Compute "/alias.dis";
    if (alias == nil || sys->bind("/blocked", "/peer", Sys->MREPL) < 0 ||
        (load Compute path) != nil || service->compute(70) != 71) {
        raise "mounted Compute result or alias load failed";
    }
    {
        alias->compute(-1);
        raise "expected remote exception";
    } exception error {
        "remote scalar owner error" =>
            ;
    }
    if (alias->compute(11) != 12) {
        raise "mounted Compute alias result failed";
    }
    sys->print("PASS typed Styx mounted function\n");
}
