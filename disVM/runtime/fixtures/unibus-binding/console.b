# Copyright (c) 2026 bhgv
# EN: A standard typed console module uses inherited descriptors with a nil display/session.
# RU: Обычный типизированный консольный модуль использует FD при nil display/session.
implement Terminalcmd;
include "terminalcmd.m";


# EN/RU: Reject actual console I/O failures, without requiring any display context.
# Проверить настоящее консольное I/O без требования display context.
check(ok: int, why: string)
{
    if (!ok) {
        raise why;
    }
}


# EN: Normal typed init with nil terminal; all byte/file operations go through ordinary Sys FD.
# args is borrowed and optional. No terminal connection, renderer or UI-capability is created.
# RU: Штатный typed init с nil terminal, операции байтов/файлов через обычные Sys FD.
# args заимствован и необязателен. Подключение терминала и UI-capability не создаётся.
init(terminal: ref Terminal->Session, args: list of string)
{
    sys := load Sys Sys->PATH;
    check(terminal == nil && args == nil, "console init unexpectedly acquired a display");
    expected := array of byte "console-fd\n";
    bytes := array[len expected] of byte;
    input := sys->fildes(0);
    check(input != nil, "stdin not inherited without a display");
    at := 0;
    while (at < len bytes) {
        count := sys->read(input, bytes[at:], len bytes - at);
        check(count > 0, "stdin ended before all console bytes");
        at += count;
    }
    check(string bytes == string expected, "stdin content changed");
    file := sys->create("console.txt", Sys->OWRITE, 8r666);
    check(file != nil, "ordinary console create: " + sys->sprint("%r"));
    check(sys->write(file, array of byte "data", 4) == 4, "ordinary console write: " + sys->sprint("%r"));
    check(sys->seek(file, big 0, Sys->SEEKSTART) == big 0, "ordinary console seek: " + sys->sprint("%r"));
    file = nil;
    file = sys->open("console.txt", Sys->OREAD);
    check(file != nil, "ordinary console reopen: " + sys->sprint("%r"));
    copy := array[4] of byte;
    check(sys->read(file, copy, 4) == 4 && string copy == "data", "ordinary file bytes changed");
    check(sys->fprint(sys->fildes(1), "PASS console stdout without display\n") > 0,
        "stdout not inherited without a display");
    check(sys->fprint(sys->fildes(2), "PASS console stderr without display\n") > 0,
        "stderr not inherited without a display");
}
