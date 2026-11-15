# Copyright (c) 2026 bhgv
# EN: Real host rwx checks through rebound guest paths; no virtual credentials.
# RU: Настоящие host rwx через перепривязанные guest paths; без virtual credentials.
implement PermissionCheck;
include "sys.m";
PermissionCheck: module { init: fn(); };
sys: Sys; # Shared immutable builtin module after initialization.


# EN/RU: Require a guest-visible condition; raise the borrowed label on failure.
# Проверить guest-visible условие; при отказе raise с переданной подписью.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN: Exercise actual opens/stat under host credentials. All local FDs retire
# with their frames. A bind must neither grant writes nor bypass directory x.
# RU: Проверить opens/stat с host credentials. Локальные FD уходят с frames.
# Bind не должен давать запись или обходить directory x.
init()
{
    sys = load Sys Sys->PATH;
    check(sys->pctl(Sys->FORKNS, nil) > 0, "detach namespace");
    output := sys->open("writeonly", Sys->OWRITE);
    check(output != nil, "write-only requires read permission");
    check(sys->read(output, array[1] of byte, 1) == -1, "write-only descriptor readable");
    check(sys->write(output, array[] of {byte 'X'}, 1) == 1, "write-only first write");
    check(sys->seek(output, big 3, Sys->SEEKSTART) == big 3, "write-only seek");
    check(sys->write(output, array[] of {byte 'Z'}, 1) == 1, "write-only seek write");
    check(sys->open("missing", Sys->OWRITE) == nil, "write open created missing file");
    check(sys->open("visible", Sys->OWRITE) == nil, "write open directory");
    check(sys->open("writeonly", Sys->OWRITE|Sys->OTRUNC) == nil, "unsupported truncation accepted");
    check(sys->create("visible/probe", Sys->OWRITE, 8r666) != nil, "ordinary create unavailable");
    check(sys->bind("readonly", "visible", Sys->MREPL) == 0, "bind readonly");
    (status, info) := sys->stat("visible/token");
    check(status == 0 && (info.mode & 8r777) == 8r444, "rwx metadata");
    fd := sys->open("visible/token", Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1 && int bytes[0] == int 'R', "read permission");
    check(sys->open("visible/token", Sys->OWRITE) == nil, "bind granted write");
    check(sys->create("visible/token", Sys->OWRITE, 8r666) == nil, "create bypassed readonly");
    check(sys->bind("nosearch", "visible", Sys->MREPL) == 0, "bind nosearch");
    check(sys->open("visible/token", Sys->OREAD) == nil, "directory search bypass");
    check(sys->stat("visible/token").t0 == -1, "stat search bypass");
    check(sys->unmount(nil, "visible") == 0, "unmount");
    check(sys->open("visible/token", Sys->OREAD) != nil, "restore visible");
    sys->print("PASS namespace host rwx: bind/read/create/search/stat/unmount\n");
}
