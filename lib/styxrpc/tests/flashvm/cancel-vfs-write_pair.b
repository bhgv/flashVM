# Copyright (c) 2026 bhgv
# EN: Check exact cancellation of a positioned write after a confirmed prefix.
# RU: Проверить точную отмену позиционной записи после подтверждённого префикса.
implement Client;
include "sys.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # Running test's namespace and clock.
nodes: Styxnodes; # Callback-file API on the owning Limbo thread.
Node, Request: import nodes;
ProbeBudgetMs: con 1800; # Bound unrelated work while a callback remains unanswered.
StopBudgetMs: con 900; # Shorter than the transport and mailbox deadlines.
PollMs: con 5; # Poll period for the movie's progress markers.

include "named_movie_upload.b";


# EN: Wait up to budget milliseconds for a marker from the movie.
# RU: Ждать маркер фильма не более budget миллисекунд.
wait_for(path: string, budget: int)
{
    started := sys->millisec();
    while (sys->millisec()-started < budget) {
        (ok, nil) := sys->stat(path);
        if (ok == 0) {
            return;
        }
        sys->sleep(PollMs);
    }
    raise "missing file-write marker " + path;
}


# EN: Write the next gate stage to the first byte of an ordinary file.
# RU: Записать следующий этап барьера в первый байт обычного файла.
signal(stage: string)
{
    gate := sys->open("/write-gate", Sys->OWRITE);
    ready := array of byte stage;
    if (gate == nil || sys->write(gate, ready, len ready) != len ready) {
        raise "cannot signal worker write";
    }
}


# EN: Return one request from node after checking its operation and byte range.
# RU: Вернуть запрос от node после проверки операции и байтового диапазона.
expect(node: ref Node, operation: string, offset: big, count: int): ref Request
{
    (request, position) := nodes->recvat(node);
    if (request == nil || request.operation != operation || position != offset || len request.buffer != count) {
        raise "wrong positioned worker request " + node.path;
    }
    return request;
}


# EN: Confirm the first write byte, cancel the suffix, finish an independent read,
# and finish the pair with another write pending.
# RU: Подтвердить первый байт записи, отменить остаток, завершить независимое
# чтение и завершить пару с ещё одной ожидающей записью.
init()
{
    sys = load Sys Sys->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    if (nodes == nil) {
        raise "cannot load Styxnodes";
    }
    nodes->init();
    slow := nodes->publishat("/slow-write", "w", big 2);
    other := nodes->publishat("/other-data", "r", big 1);
    disconnect := nodes->publishat("/disconnect-write", "w", big 2);
    start("cancel-vfs-write.swf");
    first := expect(slow, "w", big 0, 2);
    if (first.buffer[0] != byte 17 || first.buffer[1] != byte 29 || !slow.reply(first, 1)) {
        raise "wrong first write or short acknowledgement";
    }
    held := expect(slow, "w", big 1, 1);
    if (held.buffer[0] != byte 29) {
        raise "confirmed write prefix was replayed";
    }
    live := expect(other, "r", big 0, 1);
    signal("1");
    wait_for("/write-before", ProbeBudgetMs);
    signal("2");
    wait_for("/write-cancelled", StopBudgetMs);
    if (slow.reply(held, 1)) {
        raise "cancelled write accepted a late reply";
    }
    live.buffer[0] = byte 61;
    if (!other.reply(live, 1)) {
        raise "cancellation affected the independent read";
    }
    fresh := expect(slow, "w", big 0, 2);
    if (fresh.buffer[0] != byte 44 || fresh.buffer[1] != byte 45 || !slow.reply(fresh, 2)) {
        raise "fresh cursor did not complete";
    }
    wait_for("/write-finished", ProbeBudgetMs);
    slow.close();
    other.close();
    pending := expect(disconnect, "w", big 0, 2);
    if (pending.buffer[0] != byte 67 || pending.buffer[1] != byte 68) {
        raise "disconnect write did not reach the owner";
    }
    sys->print("PASS native cancel-vfs-write RPC paired VMs\n");
}
