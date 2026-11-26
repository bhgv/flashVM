# Copyright (c) 2026 bhgv
# EN: Check independent revisions and same-fid rewind across FlashVM and DisVM.
# RU: Проверить раздельные ревизии и перемотку того же fid между FlashVM и DisVM.
implement Client;
include "sys.m";
include "styxexec.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # Runtime filesystem and scheduler.
exec: Styxexec; # Movie phase signal over the existing paired mount.
nodes: Styxnodes; # Local publication shown in the live peer namespace.
Node: import nodes;
Held: con "/peer/held"; # Directory long enough for several transport replies.
Gone: con "/peer/vanish"; # Directory that will be withdrawn and recreated.
Entries: con 80; # Initial callback count in Held.

include "named_movie_upload.b";


# EN: Wait for a path to enter or leave the live namespace, giving owners time to run.
# RU: Дождаться появления или исчезновения пути, давая владельцам время на работу.
await_path(path: string, present: int)
{
    for (i := 0; i < 200; ++i) {
        (status, entry) := sys->stat(path);
        if ((status == 0) == present) {
            return;
        }
        sys->sleep(10);
    }
    raise "path did not reach the expected state: " + path;
}


# EN: Schedule the next movie phase; the callback replies before directory mutation.
# RU: Запланировать фазу фильма. Колбэк отвечает до изменения каталога.
phase(n: int)
{
    data := array[] of {byte n};
    reply := exec->call("/peer", "dirSignal", data);
    if (len reply) {
        raise "unexpected reply from directory phase";
    }
}


# EN: Read a fresh full iteration after rewind, counting the current files and new marker.
# RU: Полностью обойти каталог после перемотки, подсчитать текущие файлы и новую метку.
verify(dir: ref Sys->FD)
{
    total := 0;
    added := 0;
    for (;;) {
        (count, entries) := sys->dirread(dir);
        if (count < 0) {
            raise sys->sprint("rewound iteration failed after %d entries: %r", total);
        }
        if (!count) {
            break;
        }
        total += count;
        for (i := 0; i < count; ++i) {
            if (entries[i].name == "new") {
                ++added;
            }
        }
    }
    if (total != Entries+1 || added != 1) {
        raise "rewind did not list the current membership exactly once";
    }
}


# EN: Keep both directory fids open while the peers change their own namespace.
# RU: Удерживать оба fid каталога во время изменений собственных пространств имён.
init()
{
    sys = load Sys Sys->PATH;
    exec = load Styxexec Styxexec->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    if (exec == nil || nodes == nil) {
        raise "missing paired directory test modules";
    }
    exec->init();
    nodes->init();
    start("dirlive.swf");
    await_path("/peer/dirSignal", 1);
    held := sys->open(Held, Sys->OREAD);
    gone := sys->open(Gone, Sys->OREAD);
    if (held == nil || gone == nil) {
        raise "cannot open original directory fids";
    }
    (first, nil) := sys->dirread(held);
    if (first <= 0 || first >= Entries/2) {
        raise "first listing unexpectedly exhausted the directory";
    }

    added := nodes->publish("/tools/reread", "x");
    phase(1);
    await_path("/peer/other/added", 1);
    (second, nil) := sys->dirread(held);
    if (second <= 0 || first+second >= Entries) {
        raise "unrelated directory mutation interrupted the held cursor";
    }

    added.close();
    await_path("/tools/reread", 0);
    phase(2);
    await_path(Held+"/new", 1);
    for (;;) {
        (count, entries) := sys->dirread(held);
        if (count < 0) {
            break;
        }
        if (!count) {
            raise "changed membership silently continued an old enumeration";
        }
    }
    if (sys->seek(held, big 0, Sys->SEEKSTART) != big 0) {
        raise sys->sprint("cannot restart held directory: %r");
    }
    verify(held);
    if (sys->seek(gone, big 0, Sys->SEEKSTART) != big 0) {
        raise "local zero-offset reset should succeed on an open directory";
    }
    (stale, nil) := sys->dirread(gone);
    if (stale >= 0) {
        raise "old directory fid rebound to a replacement";
    }
    fresh := sys->open(Gone, Sys->OREAD);
    if (fresh == nil) {
        raise "replacement directory did not open";
    }
    (current, entries) := sys->dirread(fresh);
    if (current != 1 || entries[0].name != "new") {
        raise "fresh open did not see the replacement directory";
    }
    sys->print("PASS native dirlive RPC paired VMs\n");
}
