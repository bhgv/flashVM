# Copyright (c) 2026 bhgv
# EN: Use typed load on a Styx module directory and on its bound alias.
# RU: Типизированная загрузка каталога модуля Styx и его bind-алиаса.
implement Client;
include "sys.m";
include "compute.m";
Client: module { init: fn(); };
Wrong: module { compute: fn(value: big): big; };
Helper: module { offset: fn(value: int): int; };
Combined: module {
    adjust: fn(value: int): int;
    compute: fn(value: int): int;
};
Missing: module {
    adjust: fn(value: int): int;
    absent: fn(value: int): int;
};
Bad: module {
    adjust: fn(value: int): int;
    compute: fn(value: big): big;
};


# EN/RU: Check compiled import admission, normal results and a remote exception.
# Проверить допуск импорта, обычный результат и удалённое исключение.
init()
{
    sys := load Sys Sys->PATH;
    disowner := sys->open("expect-dis-owner", Sys->OREAD) != nil;
    first := 71;
    last := 12;
    if (disowner) {
        first = 113;
        last = 56;
    }
    path := "/peer/mounted/compute.dis";
    helper: Helper;
    combined: Combined;
    if (disowner) {
        helperpath := "/peer/mounted/helper.dis";
        if ((load Compute helperpath) != nil || (load Helper path) != nil) {
            raise "profile from another module was admitted";
        }
        if ((load Missing path) != nil || (load Bad path) != nil) {
            raise "partially matching import accepted";
        }
        combined = load Combined path;
        if (combined == nil || combined->adjust(70) != 64) {
            raise "multi-export module load failed";
        }
        helper = load Helper helperpath;
        if (helper == nil || helper->offset(70) != 63) {
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
        if (helper->offset(11) != 4 || combined->adjust(16) != 14 || combined->compute(1) != 48) {
            raise "alternate Styx route changed after rebind";
        }
        sys->print("PASS mounted Styx original Dis\n");
    } else {
        sys->print("PASS typed Styx mounted function\n");
    }
}
