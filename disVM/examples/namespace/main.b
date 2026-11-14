# Copyright (c) 2026 bhgv
# EN: Shared/copied guest namespaces, stable descriptors and a detached root.
# RU: Общие/копируемые guest namespace, стабильные FD и отдельный корень.
implement NamespaceDemo;
include "sys.m";
NamespaceDemo: module { init: fn(); };
sys: Sys; # Builtin loaded before native children start.


# EN/RU: Shared native return cell; guarded publication must match branch readers.
# Общая ячейка native return; guarded publication согласуется с branch readers.
Result: adt {
    value: int; # Direct builtin return destination shared by four writers.
};


# EN: Read the first byte from path or raise; FD retires with this frame.
# RU: Прочитать первый byte path либо raise; FD освобождается с frame.
read(path: string): int
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    if (fd == nil || sys->read(fd, bytes, 1) != 1) {
        raise "namespace read failed: " + path;
    }
    return int bytes[0];
}


# EN: Require condition, raising label for a failed namespace contract.
# RU: Проверить condition, выдать label при нарушении контракта namespace.
require(condition: int, label: string)
{
    if (!condition) {
        raise label;
    }
}


# EN: Optionally detach before acknowledging readiness; service bounded read
# commands until zero. Channels provide ordering, not namespace synchronization.
# RU: Опционально отделиться до ready; читать по командам до нуля. Каналы
# задают порядок, но не заменяют синхронизацию namespace.
reader(detach: int, commands: chan of int, replies: chan of int)
{
    if (detach) {
        require(sys->pctl(Sys->FORKNS, nil) > 0, "FORKNS failed");
    }
    replies <-= 1;
    for (;;) {
        command := <-commands;
        if (!command) {
            return;
        }
        if (command == 2) {
            replies <-= sys->chdir("source-a");
            continue;
        }
        replies <-= read("visible/token");
    }
}


# EN: NEWNS rebases root at cwd; virtual '..' remains at that root. This is
# path routing, not a security sandbox against host symlinks or open handles.
# RU: NEWNS берёт cwd за root; virtual '..' остаётся в корне. Это маршрутизация,
# не sandbox против host symlinks либо уже открытых handles.
rooted(done: chan of int)
{
    require(sys->pctl(Sys->FORKNS, nil) > 0, "root child FORKNS failed");
    require(sys->chdir("source-a") == 0, "root child chdir failed");
    require(sys->pctl(Sys->NEWNS, nil) > 0, "NEWNS failed");
    require(read("/token") == int 'A' && read("/../../token") == int 'A', "NEWNS root changed");
    done <-= 1;
}


# EN: Publish namespace return directly into a shared field after start handshake.
# done acknowledges completion; cell/start/done remain rooted by caller frames.
# RU: Писать namespace return прямо в shared field после start. done сообщает
# завершение; cell/start/done укоренены caller frames.
scalar_writer(cell: ref Result, start: chan of int, done: chan of int)
{
    <-start;
    for (index := 0; index < 2000; index++) {
        cell.value = sys->pctl(0, nil);
    }
    done <-= 1;
}


# EN/RU: Read shared returns while four native writers execute; join all replies.
# Читать shared returns при работе четырёх native writers; дождаться всех ответов.
scalar_returns()
{
    cell := ref Result(0);
    start := chan of int;
    done := chan of int;
    for (index := 0; index < 4; index++) {
        spawn scalar_writer(cell, start, done);
    }
    for (index = 0; index < 4; index++) {
        start <-= 1;
    }
    completed := 0;
    while (completed < 4) {
        alt {
            <-done => completed++;
            * => require(cell.value >= 0, "shared native return corrupted");
        }
    }
    require(cell.value > 0, "shared namespace PID missing");
}


# EN: Run from the prepared example directory containing source-a/source-b/visible.
# Test real open/read/stat/create, inherited sharing, copy isolation and root reset.
# RU: Запускать из prepared directory с source-a/source-b/visible. Проверить
# реальные open/read/stat/create, наследование, изоляцию копии и новый корень.
init()
{
    sys = load Sys Sys->PATH;
    require(sys->pctl(0, nil) > 0, "pctl PID failed");
    require(sys->pctl(Sys->NEWFD, nil) == -1, "unsupported pctl accepted");
    held := sys->open("visible/token", Sys->OREAD);
    require(held != nil, "initial descriptor missing");
    require(sys->bind("source-a", "visible", Sys->MREPL) == 0, "initial bind failed");
    require(read("visible/token") == int 'A', "bind not routed");
    (status, info) := sys->stat("visible/token");
    require(status == 0 && info.length == big 1, "bound stat failed");
    bytes := array[1] of byte;
    require(sys->read(held, bytes, 1) == 1 && bytes[0] == byte 'V', "bind redirected existing FD");
    held = nil;
    output := sys->create("visible/created", Sys->OWRITE, 8r666);
    bytes[0] = byte 'C';
    require(output != nil && sys->write(output, bytes, 1) == 1, "bound create failed");
    output = nil;
    require(read("source-a/created") == int 'C', "create did not reach source");
    require(sys->bind("source-b", "visible", Sys->MBEFORE) == -1, "union flag accepted");
    require(sys->bind("source-a/token", "visible", Sys->MREPL) == -1, "mixed kind accepted");
    require(sys->chdir("source-a/token") == -1, "file accepted as cwd");
    require(read("visible/token") == int 'A', "failure changed namespace");
    shared := chan of int;
    copied := chan of int;
    shared_reply := chan of int;
    copied_reply := chan of int;
    spawn reader(0, shared, shared_reply);
    spawn reader(1, copied, copied_reply);
    require(<-shared_reply == 1 && <-copied_reply == 1, "reader setup failed");
    require(sys->bind("source-b", "visible", Sys->MREPL) == 0, "rebind failed");
    shared <-= 1;
    copied <-= 1;
    require(<-shared_reply == int 'B' && <-copied_reply == int 'A', "namespace inheritance mismatch");
    require(sys->unmount("source-a", "visible") == -1, "wrong source unmounted");
    require(sys->unmount("source-b", "visible") == 0, "filtered unmount failed");
    require(read("visible/token") == int 'V', "unmount did not restore target");
    require(sys->bind("source-a", "visible", Sys->MREPL) == 0, "second bind failed");
    require(sys->unmount(nil, "visible") == 0, "unfiltered unmount failed");
    require(sys->unmount(nil, "visible") == -1, "absent unmount accepted");
    shared <-= 2;
    require(<-shared_reply == 0 && read("token") == int 'A', "shared cwd not visible");
    require(sys->chdir("..") == 0, "shared cwd reset failed");
    shared <-= 0;
    copied <-= 0;
    done := chan of int;
    spawn rooted(done);
    require(<-done == 1, "root child failed");
    require(sys->chdir("source-a") == 0 && read("token") == int 'A', "relative cwd failed");
    require(sys->chdir("..") == 0 && read("visible/token") == int 'V', "parent cwd failed");
    scalar_returns();
    sys->print("NAMESPACE shared=B copied=A descriptor=V create=C root=A restored=V\n");
}
