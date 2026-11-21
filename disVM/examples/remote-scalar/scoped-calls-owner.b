# Copyright (c) 2026 bhgv
# EN: Read the caller's mounted context through ordinary Sys operations.
# RU: Читать смонтированный контекст клиента обычными операциями Sys.
implement Scopedcalls;
include "sys.m";
include "scoped-calls.m";

Caller: con "/.prog/.self/"; # Alias set in the private namespace of one call.
saved: ref Sys->FD; # An unread caller fid from the first completed inspection.


# EN: Read at most limit bytes from a single context file or report owner failure.
# RU: Прочитать не больше limit байт файла контекста или сообщить об ошибке.
contents(sys: Sys, path: string, limit: int): string
{
    handle := sys->open(path, Sys->OREAD);
    if (handle == nil) {
        raise "caller file unavailable: " + path;
    }
    data := array[limit] of byte;
    n := sys->read(handle, data, len data);
    if (n <= 0) {
        raise "caller read failed: " + path;
    }
    return string data[0:n];
}


# EN: Verify the selected PID, grants and inherited FD; return its four bytes as a code.
# RU: Сверить выбранный PID, выдачи и унаследованный FD; вернуть код четырёх байтов.
inspect(fail: int): (int,big)
{
    sys := load Sys Sys->PATH;
    if (sys->pctl(Sys->FORKNS, nil) <= 0) {
        raise "failed to copy scoped namespace";
    }
    pid := int contents(sys, Caller + ".pid", 32);
    if (pid <= 0 || contents(sys, Caller + ".vfs/env/CLIENT", 8) != "/.vfs" ||
        contents(sys, Caller + ".var/counter", 8) != "/.var") {
        raise "wrong caller scope";
    }
    if (fail != 0) {
        raise "requested scoped owner failure";
    }
    value := contents(sys, Caller + ".fd/3", 4);
    if (value == "left") {
        saved = sys->open(Caller + ".pid", Sys->OREAD);
        if (saved == nil) {
            raise "could not save the scoped fid";
        }
        return (pid, big 1);
    }
    if (value == "next") {
        return (pid, big 2);
    }
    if (value == "last") {
        return (pid, big 3);
    }
    raise "wrong inherited caller fd data";
}


# EN: Look for a leaked caller alias from a control call with no scope.
# RU: Проверить отсутствие утечки алиаса в контрольном вызове без контекста.
baseline(flag: int): int
{
    sys := load Sys Sys->PATH;
    if (sys->open(Caller + ".pid", Sys->OREAD) != nil) {
        raise "caller scope leaked into unscoped export";
    }
    if (saved != nil) {
        data := array[32] of byte;
        if (sys->read(saved, data, len data) > 0) {
            raise "saved caller fid remained readable after the reply";
        }
        saved = nil;
    }
    return flag;
}
