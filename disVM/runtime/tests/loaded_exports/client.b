# Copyright (c) 2026 bhgv
# EN: Compare loaded instance catalogues and shared state across an atomic image replacement.
# RU: Сверить каталоги и общее состояние экземпляров при атомарной замене образа.
implement Client;
include "sys.m";
include "vfsnative.m";

Client: module {
    init: fn();
};

Attempts: con 300; # Maximum checks for the host's replacement marker.
Sleepms: con 10; # Delay between checks, in milliseconds.


# EN: Wait for the test runner to replace the file while the original anchor is live.
# RU: Дождаться замены файла от теста при живом исходном экземпляре.
waitforreplace(sys: Sys)
{
    for (i := 0; i < Attempts; i++) {
        (status, nil) := sys->stat("replacement-ready");
        if (status >= 0) {
            return;
        }
        sys->sleep(Sleepms);
    }
    raise "replacement did not arrive";
}


# EN: Require each supported shape exactly once, regardless of the compiler's order.
# RU: Проверить по одному имени каждой поддерживаемой схемы в любом порядке.
expected(sys: Sys, names: string): int
{
    (count, fields) := sys->tokenize(names, "\t");
    mask := 0;
    for (; fields != nil; fields = tl fields) {
        case hd fields {
        "add" => mask |= 1;
        "current" => mask |= 2;
        "label" => mask |= 4;
        "ports" => mask |= 8;
        * => return 0;
        }
    }
    return count == 4 && mask == 15;
}


# EN: Emit an exact private offer sequence for the host-side wire check.
# RU: Вывести точную последовательность предложений для проверки на хосте.
printprofile(sys: Sys, label: string, profile: array of byte)
{
    if (profile == nil) {
        raise "missing owner profile";
    }
    line := label + " ";
    for (i := 0; i < len profile; i++) {
        line += sys->sprint("%02x", int profile[i]);
    }
    sys->print("%s\n", line);
}


# EN: Read compiler-derived methods and invoke typed views of each exact anchor.
# RU: Прочитать методы компилятора и вызвать представления точных экземпляров.
init()
{
    sys := load Sys Sys->PATH;
    routes := load Vfsnative Vfsnative->PATH;
    held := routes->scalarloadanchor("target.dis");
    names := routes->scalarloadedexports(held);
    if (held == nil || !expected(sys, names) ||
        routes->scalarloadedexports(nil) != nil ||
        routes->scalarloadedprofile(nil) != nil) {
        raise "invalid original catalogue";
    }
    overflow := routes->scalarloadanchor("overflow.dis");
    if (overflow == nil || routes->scalarloadedexports(overflow) != nil ||
        routes->scalarloadedprofile(overflow) != nil) {
        raise "accepted a partial overflowing catalogue";
    }
    snapshot := routes->scalarloadedprofile(held);
    sys->print("READY original module\n");
    waitforreplace(sys);
    fresh := routes->scalarloadanchor("target.dis");
    if (fresh == nil || routes->scalarloadedexports(fresh) != "other" ||
        routes->scalarloadedexports(held) != names) {
        raise "catalogue changed under old anchor";
    }
    sys->print("compiled-names %s\n", names);
    printprofile(sys, "original-profile", snapshot);
    snapshot[0] = byte 0;
    printprofile(sys, "held-profile", routes->scalarloadedprofile(held));
    printprofile(sys, "fresh-profile", routes->scalarloadedprofile(fresh));
    (nil, add, nil, nil, nil, nil, nil) := routes->scalarloadedview(held, "add", nil);
    (nil, current, nil, nil, nil, nil, nil) := routes->scalarloadedview(held, "current", nil);
    (nil, other, nil, nil, nil, nil, nil) := routes->scalarloadedview(fresh, "other", nil);
    if (add == nil || current == nil || other == nil ||
        add->invoke(7) != 12 || current->invoke(0) != 12 || other->invoke(7) != 0) {
        raise "wrong linked module instance";
    }
    sys->print("PASS exact compiled export catalogue and shared module state\n");
}
