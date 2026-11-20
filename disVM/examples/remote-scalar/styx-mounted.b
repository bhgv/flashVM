# Copyright (c) 2026 bhgv
# EN: Use typed load on a Styx module directory and on its bound alias.
# RU: Типизированная загрузка каталога модуля Styx и его bind-алиаса.
implement Client;
include "sys.m";
include "compute.m";
Client: module { init: fn(); };
Wrong: module { compute: fn(value: big): big; };
Helper: module { adjust: fn(value: int): int; };


# EN/RU: Check compiled import admission, normal results and a remote exception.
# Проверить допуск импорта, обычный результат и удалённое исключение.
init()
{
    sys := load Sys Sys->PATH;
    disowner := sys->open("expect-dis-owner", Sys->OREAD) != nil;
    first := 71;
    last := 12;
    if (disowner) {
        first = 112;
        last = 55;
    }
    path := "/peer/mounted/compute.dis";
    helper: Helper;
    if (disowner) {
        helperpath := "/peer/mounted/helper.dis";
        if ((load Compute helperpath) != nil || (load Helper path) != nil) {
            raise "profile from another module was admitted";
        }
        helper = load Helper helperpath;
        if (helper == nil || helper->adjust(70) != 63) {
            raise "alternate Styx module failed";
        }
    }
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
        (load Compute path) != nil || service->compute(70) != first) {
        raise "mounted Compute result or alias load failed";
    }
    {
        alias->compute(-1);
        raise "expected remote exception";
    } exception error {
        "remote scalar owner error" =>
            ;
    }
    if (alias->compute(11) != last) {
        raise "mounted Compute alias result failed";
    }
    if (disowner) {
        if (helper->adjust(11) != 4) {
            raise "alternate Styx route changed after rebind";
        }
        sys->print("PASS mounted Styx original Dis\n");
    } else {
        sys->print("PASS typed Styx mounted function\n");
    }
}
