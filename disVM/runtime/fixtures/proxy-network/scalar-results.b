# Copyright (c) 2026 bhgv
# EN: Builtin scalar results assigned directly into shared heap cells under live readers.
# RU: Builtin scalar results записываются прямо в shared heap cells при live readers.
implement ProxyScalarResults;
include "sys.m";
include "processctl.m";
ProxyScalarResults: module { init: fn(); };


# EN/RU: Scalar-only shared payload; compiler may target builtin return directly at fields.
# Scalar-only shared payload; compiler может направить builtin return прямо в поля.
Cells: adt
{
    pid: int; # Positive process identity, including direct native result writes.
    clock: int; # Monotonic native clock value, also a direct builtin result.
    slept: int; # Sys.sleep(0) outcome; must remain zero.
};


# EN/RU: Publish native results directly into shared fields on this original writer.
# Публиковать native results прямо в shared fields с исходного writer.
writer(cells: ref Cells, ready, done: chan of int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    ready <-= 1;
    for (round := 0; round < 1000; round++) {
        cells.pid = control->pid();
        cells.clock = sys->millisec();
        cells.slept = sys->sleep(0);
    }
    done <-= 1;
}


# EN/RU: Keep branch readers active while native builtin writers target the same cells.
# Сохранять branch readers, пока native builtin writers пишут те же cells.
init()
{
    sys := load Sys Sys->PATH;
    cells := ref Cells(0, 0, 0);
    ready := chan[4] of int;
    done := chan[4] of int;
    for (index := 0; index < 4; index++) {
        spawn writer(cells, ready, done);
    }
    for (index = 0; index < 4; index++) {
        <-ready;
    }
    for (round := 0; round < 20000; round++) {
        if (cells.pid < 0 || cells.clock < 0 || cells.slept != 0) {
            raise "shared native scalar result corrupted";
        }
    }
    for (index = 0; index < 4; index++) {
        <-done;
    }
    if (cells.pid <= 0 || cells.clock <= 0 || cells.slept != 0) {
        raise "shared native scalar result missing";
    }
    sys->fprint(sys->fildes(2), "PASS guarded native scalar results: 4 writers and shared branch readers\n");
}
