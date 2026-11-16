# Copyright (c) 2026 bhgv
# EN: Real native guest captured roots through Sys.bind, Vfsctl, Vfsexport and namespace copies.
# RU: Настоящие native guest captured roots через Sys.bind, Vfsctl, Vfsexport и namespace copies.
implement Check;
include "sys.m";
include "vfsexport.m";
include "vfsctl.m";
Check: module { init: fn(); };
sys: Sys; # Initialized ordinary filesystem API.
api: Vfsexport; # Native root capability operations, shareable through typed spawn arguments.
ctl: Vfsctl; # Layer capture capabilities and indexed stack mutations.


# EN/RU: Raise explicit contract failure in both interpreter and JIT.
# Явный contract failure в interpreter и JIT.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Use ordinary guest FD operations, not a native fixture read bypass.
# Использовать ordinary guest FD operations, не native fixture bypass.
value(path: string): string
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "captured read " + path);
    return string bytes;
}


# EN/RU: Separate permanent guest thread mounts a shared captured root only in its copied namespace.
# Отдельный permanent guest thread монтирует shared captured root только в copied namespace.
child(root: ref Vfsexport->Root, done: chan of int)
{
    check(sys->pctl(Sys->FORKNS, nil) > 0 && api->mount(root, "isolated", 0, 0) == 0 &&
          value("isolated/item") == "A", "shared captured root/copy namespace");
    done <-= 1;
}


# EN/RU: Capture before source replacement, then require pinned identity and live overlay policy.
# Захватить до source replacement; требовать pinned identity и live overlay policy.
init()
{
    sys = load Sys Sys->PATH;
    api = load Vfsexport Vfsexport->PATH;
    ctl = load Vfsctl Vfsctl->PATH;
    check(sys != nil && api != nil && ctl != nil, "load capture APIs");
    ctl->init();
    (status, nil, layers) := ctl->layers("remote/dir");
    check(status == Vfsctl->Ready && len layers == 1 &&
          ((hd layers).capabilities & Vfsctl->Capturebind) != 0 &&
          ((hd layers).capabilities & Vfsctl->Stablebind) == 0, "dynamic capture capability observation");
    check(sys->bind("remote/flip", "plain", Sys->MREPL) == -1 && value("plain/local") == "P",
          "Sys.bind published a different captured kind");
    check(ctl->bind("remote/flip", "bound", Vfsctl->MREPL, 0) == -1 && value("bound/local") == "Q",
          "Vfsctl published a different captured kind");
    check(ctl->bind("remote/dir", "remote/flip", Vfsctl->MBEFORE, 0) == -1,
          "Vfsctl union accepted a different captured target kind");
    check(sys->bind("remote/dir", "plain", Sys->MREPL) == 0 && value("plain/item") == "A", "Sys.bind native directory capture");
    check(ctl->bind("remote/dir", "bound", Vfsctl->MREPL, 0) == 0 && value("bound/item") == "A", "Vfsctl directory capture");
    root := api->capture("remote/dir", 0, Vfsexport->Read|Vfsexport->Execute);
    check(root != nil, "Vfsexport native capture");
    prepared := api->overlay(root, nil, "/hidden", 0, Vfsexport->Tree);
    check(prepared != nil && api->mount(prepared, "view", 0, 0) == 0 &&
          value("view/item") == "A", "prepared capture/mount");
    (found, nil) := sys->stat("view/hidden");
    check(found == -1, "hidden overlay escaped capture");
    check(ctl->bind("plain/item", "saved", Vfsctl->MREPL, 0) == 0 && value("saved") == "A", "captured file below captured directory");
    (status, nil, layers) = ctl->layers("plain");
    check(status == Vfsctl->Ready && ((hd layers).capabilities & Vfsctl->Stablebind) != 0,
          "captured root did not expose fixed-dot identity");
    fd := sys->open("remote/ctl", Sys->OWRITE);
    check(fd != nil && sys->write(fd, array[] of {byte 'B'}, 1) == 1, "replace original source");
    check(value("remote/dir/item") == "B" && value("plain/item") == "A" &&
          value("bound/item") == "A" && value("view/item") == "A" && value("saved") == "A",
          "captured root reopened old pathname");
    done := chan of int;
    spawn child(root, done);
    check(<-done == 1 && value("isolated/local") == "I", "captured child mount changed parent namespace");
    check(sys->unmount(nil, "remote") == 0 && value("view/item") == "A" && value("saved") == "A",
          "source unmount dropped captured roots");
    sys->print("PASS guest captured subroots Sys.bind Vfsctl Vfsexport policy namespace lifetime\n");
}
