# Copyright (c) 2026 bhgv
# EN: Check the same fixed-length VFS contract in both directions of the stock pair.
# RU: Проверить один контракт VFS для фиксированной длины в обоих направлениях штатной пары.
implement Client;
include "sys.m";
include "styxnodes.m";
include "styxexec.m";

Client: module { init: fn(); };
sys: Sys; # Runtime filesystem and scheduler.
nodes: Styxnodes; # Local publication support.
Node: import nodes;
exec: Styxexec; # Reverse execute client.
Length: con big 4294967304; # Sparse fixture size, above the UInt32 range.
Path: con "/tools/positioned"; # Limbo-published peer file.

include "named_movie_upload.b";


# EN: Test the held local provider's seek and I/O bounds without triggering a callback.
# RU: Проверить смещения и границы удержанного провайдера без вызова колбэка.
local_bounds()
{
    fd := sys->open(Path, Sys->ORDWR);
    if (fd == nil) {
        raise "cannot open local positioned node";
    }
    (ok, info) := sys->fstat(fd);
    bytes := array[2] of byte;
    if (ok < 0 || info.length != Length ||
        sys->seek(fd, big -1, Sys->SEEKEND) != Length-big 1 ||
        sys->write(fd, bytes, 2) >= 0 ||
        sys->seek(fd, big 1, Sys->SEEKRELA) != Length ||
        sys->read(fd, bytes, 2) != 0 ||
        sys->seek(fd, big -1, Sys->SEEKSTART) >= big 0 ||
        sys->seek(fd, big 0, Sys->SEEKRELA) != Length) {
        raise "local positioned file did not enforce the declared range";
    }
}


# EN: An executable permission bit on the directory must not select a callback.
# RU: Бит исполнения у каталога не должен выбирать колбэк.
directory_refuses(): int
{
    {
        response := exec->call("/peer", "view", nil);
        return 0;
    } exception e {
    "*" =>
        return 1;
    }
}


# EN: Seek and use the FlashVM file through the ordinary mounted DisVM Sys interface.
# RU: Позиционировать и использовать файл FlashVM через штатный Sys подключённой VFS.
reverse()
{
    fd := sys->open("/peer/view/file", Sys->ORDWR);
    other := sys->open("/peer/view/file", Sys->OREAD);
    if (fd == nil || other == nil) {
        raise "cannot open the FlashVM positioned node";
    }
    (ok, info) := sys->fstat(fd);
    bytes := array[6] of byte;
    if (ok < 0 || info.length != Length ||
        sys->seek(fd, big -2, Sys->SEEKEND) != Length-big 2) {
        raise "incorrect FlashVM length or end-relative position";
    }
    bytes[0] = byte 202;
    bytes[1] = byte 0;
    if (sys->write(fd, bytes, 2) != 2) {
        raise sys->sprint("FlashVM positioned write failed: %r");
    }
    if (sys->read(fd, bytes, 3) != 0) {
        raise "FlashVM allowed data past EOF";
    }
    if (sys->seek(fd, big -3, Sys->SEEKRELA) != Length-big 3) {
        raise "FlashVM fid advanced after rejected I/O";
    }
    got := sys->read(fd, bytes, 6);
    if (got != 3 || bytes[0] != byte 33 || bytes[1] != byte 202 || bytes[2] != byte 0) {
        raise sys->sprint("incorrect FlashVM binary tail (%d bytes: %d,%d,%d): %r",
            got, int bytes[0], int bytes[1], int bytes[2]);
    }
    if (sys->read(other, bytes, 1) != 1 || bytes[0] != byte 5) {
        raise "second FlashVM fid did not start at zero";
    }
    beyond := sys->open("/peer/view/file", Sys->OWRITE);
    if (beyond == nil || sys->seek(beyond, big -1, Sys->SEEKEND) != Length-big 1 ||
        sys->write(beyond, bytes, 2) >= 0) {
        raise "FlashVM accepted a write crossing EOF";
    }
    if (!directory_refuses()) {
        raise "FlashVM invoked a directory";
    }
    bytes[0] = byte 23;
    reply := exec->call("/peer", "view/file", bytes[:1]);
    if (len reply != 1 || reply[0] != byte 24) {
        raise "positioned FlashVM execution failed";
    }
}


# EN: Start the source-built movie, serve its four calls, then use the FlashVM-published file.
# RU: Запустить собранный SWF, обслужить четыре вызова и обратиться к файлу FlashVM.
init()
{
    sys = load Sys Sys->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    exec = load Styxexec Styxexec->PATH;
    handler := load StyxnodePositionedHandler "positioned-handler.dis";
    if (nodes == nil || exec == nil || handler == nil) {
        raise "cannot load positioned node support";
    }
    nodes->init();
    exec->init();
    file := nodes->publishat(Path, "rwx", Length);
    done := nodes->publishat("/tools/positioned-done", "x", big 0);
    local_bounds();
    start("positioned.swf");
    for (i := 0; i < 4; ++i) {
        if (!nodes->dispatchat(file, handler)) {
            raise "positioned callback reply was rejected";
        }
    }
    (mark, offset) := nodes->recvat(done);
    if (offset != big 0 || len mark.buffer || !done.reply(mark, 0)) {
        raise "no completion from the FlashVM reader";
    }
    reverse();
    done.close();
    file.close();
    sys->print("PASS native positioned RPC paired VMs\n");
}
