# Copyright (c) 2026 bhgv
# EN: A bound instance uses its own immutable launch metadata and provider-first live VFS.
# RU: Bound instance использует собственные immutable launch metadata и provider-first live VFS.
implement Value;
include "value.m";
include "sys.m";
include "launchcontext.m";


# EN/RU: Inspect the actual instance through a builtin, call its unbound dependency, and return the selected value.
# No caller metadata is edited; RESULT_EXTRA selects only the fixture's expected per-instance discriminator.
# Проверить instance через builtin, вызвать unbound dependency; RESULT_EXTRA — лишь discriminator теста.
value(): int
{
    sys := load Sys Sys->PATH;
    launch := load Launchcontext Launchcontext->PATH;
    if (launch == nil || launch->name() != "/app.dis") {
        raise "module lost accepted launch label";
    }
    arguments := launch->arguments();
    if (len arguments != 3 || int arguments[0] != 77 || int arguments[1] != 0 ||
        int arguments[2] < 1 || int arguments[2] > 2) {
        raise "module used caller launch arguments";
    }
    data := sys->open("/data", Sys->OREAD);
    bytes := array[32] of byte;
    n := sys->read(data, bytes, len bytes);
    if (n != 9 || string bytes[:n] != "execution") {
        raise "module metadata binding changed provider-first VFS";
    }
    dependency := load Value "/dep.dis";
    if (dependency == nil) {
        return -1;
    }
    return dependency->value() + RESULT_EXTRA;
}
