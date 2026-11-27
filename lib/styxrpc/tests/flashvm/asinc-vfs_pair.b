# Copyright (c) 2026 bhgv
# EN: Hold a remote worker read while the owner advances the movie over native Styx.
# RU: Удерживать чтение потока, пока владелец продвигает фильм через штатный Styx.
implement Client;
include "sys.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # System services for the running test.
nodes: Styxnodes; # Owns the gated file publication.
Node: import nodes;

include "named_movie_upload.b";


# EN: Read the playback frame count from a fresh state snapshot.
# RU: Получить число тактов плеера из нового снимка состояния.
ticks(): int
{
    fd := sys->open("/peer/.state", Sys->OREAD);
    bytes := array[4096] of byte;
    count := sys->read(fd, bytes, len bytes);
    if (count < 1) {
        raise "cannot read player state";
    }
    (n, lines) := sys->tokenize(string bytes[:count], "\n");
    for (; lines != nil; lines = tl lines) {
        line := hd lines;
        if (len line > 6 && line[:6] == "ticks=") {
            return int line[6:];
        }
    }
    raise "missing player ticks";
}


# EN: Hold the read for two additional frames, add a union branch and verify the worker write there.
# RU: Удержать чтение два новых кадра, добавить ветвь union и проверить запись потока в неё.
init()
{
    sys = load Sys Sys->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    if (nodes == nil) {
        raise "cannot load Styxnodes";
    }
    nodes->init();
    node := nodes->publishat("/tools/input", "r", big 1);
    start("asinc-vfs.swf");
    (request, offset) := nodes->recvat(node);
    if (request.operation != "r" || offset != big 0 || len request.buffer != 1) {
        raise "wrong worker read";
    }
    before := ticks();
    for (i := 0; i < 100 && ticks() < before + 2; ++i) {
        sys->sleep(10);
    }
    if (i == 100) {
        raise "player frames stopped during a worker read";
    }
    if (sys->bind("/front", "/tools", Sys->MBEFORE) < 0) {
        raise sys->sprint("cannot add current VFS branch: %r");
    }
    request.buffer[0] = byte 42;
    if (!node.reply(request, 1)) {
        raise "worker read did not accept the reply";
    }
    for (i = 0; i < 160; ++i) {
        (ok, info) := sys->stat("/ack");
        if (ok == 0) {
            fd := sys->open("/front/result", Sys->OREAD);
            bytes := array[1] of byte;
            if (sys->read(fd, bytes, 1) != 1 || bytes[0] != byte 42) {
                raise "worker did not write the confirmed byte";
            }
            node.close();
            sys->print("PASS native asinc-vfs RPC paired VMs\n");
            return;
        }
        sys->sleep(10);
    }
    raise "worker failed to complete or borrower cwd changed";
}
