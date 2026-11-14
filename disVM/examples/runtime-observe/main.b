# Copyright (c) 2026 bhgv
# EN: CLI reader for live resident metadata; no target execution or heap dereference.
# RU: CLI reader live resident metadata; без target execution либо heap dereference.
implement RuntimeObserve;
include "sys.m";
include "processctl.m";
RuntimeObserve: module { init: fn(); };


# EN/RU: Print copied cards from this VM until end; refuse unavailable data explicitly.
# Вывести copied cards этой VM до end; явно отказать unavailable data.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    sys->print("NATIVEOBSERVER pid=%d scope=residents\n", control->pid());
    cursor := big 0;
    for (;;) {
        (status, id, pid, done) := control->resident(cursor);
        if (status == 0) {
            break;
        }
        if (status != 1 || id <= cursor || pid <= 0) {
            raise "live resident observation unavailable";
        }
        sys->print("NATIVERESIDENT id=%bd pid=%d done=%d\n", id, pid, done);
        cursor = id;
    }
}
