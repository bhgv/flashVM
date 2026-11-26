# Copyright (c) 2026 bhgv
# EN: Publish Limbo nodes and verify ordinary VFS access from the source-built SWF.
# RU: Опубликовать узлы Limbo и проверить обычный доступ VFS из собранного SWF.
implement Client;
include "sys.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # Runtime filesystem and scheduler.
nodes: Styxnodes; # Local publication module.
Node: import nodes;
Path: con "/tools/mail"; # Duplex callback-backed file.

include "named_movie_upload.b";


# EN: Attempt publication and verify that path or permission validation rejects it.
# RU: Попытаться опубликовать узел и проверить отказ по пути либо правам.
rejected(path, mode: string): int
{
    {
        node := nodes->publish(path, mode);
        node.close();
    } exception e {
    "*" =>
        return 1;
    }
    return 0;
}


# EN: Issue a local one-byte read on the borrowed held file and return its result.
# RU: Прочитать один байт из заимствованного открытого файла и передать результат.
probe(held: ref Sys->FD, result: chan of int)
{
    result <-= sys->read(held, array[1] of byte, 1);
}


# EN: Wake two blocked readers by explicit refusal and owner withdrawal.
# RU: Разбудить двух ожидающих читателей явным отказом и снятием публикации.
retire(node: ref Node, held: ref Sys->FD)
{
    result := chan of int;
    spawn probe(held, result);
    item := node.recv();
    if (item.operation != "r" || node.reply(item, 2) || !node.refuse(item) || <-result >= 0) {
        raise "wrong count or refusal was handled incorrectly";
    }
    spawn probe(held, result);
    item = node.recv();
    node.close();
    node.close();
    if (<-result >= 0 || node.reply(item, 0)) {
        raise "closing the owner did not reject the blocked reader";
    }
}


# EN: Validate modes, serve the movie and prove a held file cannot reach a replacement.
# RU: Проверить режимы, обслужить SWF и убедиться, что старый файл не обращается к замене.
init()
{
    sys = load Sys Sys->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    handler := load StyxnodeHandler "nodes-handler.dis";
    if (nodes == nil || handler == nil) {
        raise "cannot load Styxnodes or its handler";
    }
    nodes->init();
    if (!rejected("/nodes.swf", "r") || !rejected("/tools/invalid", "rr") ||
        !rejected("/tools/invalid", "") || !rejected("/.execctl", "w")) {
        raise "invalid publication was accepted";
    }
    ro := nodes->publish("/tools/ro", "r");
    wo := nodes->publish("/tools/wo", "w");
    node := nodes->publish(Path, "rwx");
    done := nodes->publish("/tools/done", "x");
    held := sys->open(Path, Sys->ORDWR);
    (ok, info) := sys->fstat(held);
    if (held == nil || ok < 0 || (info.mode & 8r777) != 8r777 ||
        sys->open("/tools/ro", Sys->OWRITE) != nil || sys->open("/tools/wo", Sys->OREAD) != nil ||
        !rejected(Path, "rw")) {
        raise "incorrect VFS permissions or ownership";
    }
    start("nodes.swf");
    for (n := 0; n < 5; ++n) {
        if (!node.dispatch(handler)) {
            raise "cannot respond to FlashVM file operation";
        }
    }
    mark := done.recv();
    if (mark.operation != "x" || len mark.buffer != 1 || mark.buffer[0] != byte 91 ||
        !done.reply(mark, 0) || done.refuse(mark)) {
        raise "FlashVM did not confirm the callback results";
    }
    done.close();
    ro.close();
    wo.close();
    retire(node, held);
    for (i := 0; i < 100; ++i) {
        (ok, info) = sys->stat(Path);
        if (ok < 0) {
            break;
        }
        sys->sleep(10);
    }
    if (ok >= 0) {
        raise "closed publication is still visible";
    }
    replacement := nodes->publish(Path, "r");
    bytes := array[1] of byte;
    if (sys->read(held, bytes, len bytes) >= 0) {
        raise "retired file still reaches an owner";
    }
    replacement.close();
    sys->print("PASS native nodes RPC paired VMs\n");
}
