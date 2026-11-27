# Copyright (c) 2026 bhgv
# EN: Recognize a retired positioned read ticket and preserve a neighboring in-flight read.
# RU: Проверить снятие билета позиционного чтения и сохранение соседнего ожидающего чтения.
implement Client;
include "sys.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # Running test's filesystem and monotonic millisecond clock.
nodes: Styxnodes; # Callback-file API on the owning Limbo thread.
Node, Request: import nodes;
ProbeBudgetMs: con 1800; # Bound observation of unrelated work with both reads held.
StopBudgetMs: con 900; # Shorter than the stock Styx transport deadline.
PollMs: con 5; # Pace filesystem marker polling.

include "named_movie_upload.b";


# EN: Wait at most budget milliseconds for a marker created by the movie.
# RU: Ждать маркер фильма не дольше budget миллисекунд.
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
    raise "missing file-read marker " + path;
}


# EN: Publish the next single-byte gate stage at the same path and offset.
# RU: Записать следующий байтовый этап барьера по тому же пути и смещению.
signal(stage: string)
{
    gate := sys->open("/read-gate", Sys->OWRITE);
    ready := array of byte stage;
    if (gate == nil || sys->write(gate, ready, len ready) != len ready) {
        raise "cannot signal worker read";
    }
}


# EN: Receive a positioned read from node and validate the exact range before replying.
# RU: Получить позиционное чтение node и проверить точный диапазон перед ответом.
expect(node: ref Node, offset: big, count: int): ref Request
{
    (request, position) := nodes->recvat(node);
    if (request == nil || request.operation != "r" || position != offset || len request.buffer != count) {
        raise "wrong positioned worker read " + node.path;
    }
    return request;
}


# EN: Send one confirmed output byte for a live request; fail if it has retired.
# RU: Вернуть байт подтверждённых данных по живому запросу; при снятом запросе завершиться с ошибкой.
reply(node: ref Node, request: ref Request, value: byte)
{
    request.buffer[0] = value;
    if (!node.reply(request, 1)) {
        raise "live worker read refused reply " + node.path;
    }
}


# EN: Acknowledge a prefix, cancel one held read, serve a new cursor, then exit with another read pending.
# RU: Подтвердить префикс, отменить одно чтение, обслужить новый курсор и выйти с ожидающим чтением.
init()
{
    sys = load Sys Sys->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    if (nodes == nil) {
        raise "cannot load Styxnodes";
    }
    nodes->init();
    slow := nodes->publishat("/slow-data", "r", big 2);
    other := nodes->publishat("/other-data", "r", big 1);
    disconnect := nodes->publishat("/disconnect-data", "r", big 1);
    start("cancel-vfs-read.swf");
    reply(slow, expect(slow, big 0, 2), byte 7);
    held := expect(slow, big 1, 1);
    live := expect(other, big 0, 1);
    signal("1");
    wait_for("/read-before", ProbeBudgetMs);
    signal("2");
    wait_for("/read-cancelled", StopBudgetMs);
    if (slow.reply(held, 0)) {
        raise "cancelled read accepted a late reply";
    }
    reply(other, live, byte 61);
    reply(slow, expect(slow, big 0, 1), byte 82);
    wait_for("/read-finished", ProbeBudgetMs);
    slow.close();
    other.close();
    expect(disconnect, big 0, 1);
    sys->print("PASS native cancel-vfs-read RPC paired VMs\n");
}
