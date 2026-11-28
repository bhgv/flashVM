# Copyright (c) 2026 bhgv
# EN: Use the same native Limbo contract with the stock movie and the independent C provider.
# RU: Проверка одного контракта Limbo со штатным фильмом и независимым провайдером C.
implement Client;
include "sys.m";

Client: module { init: fn(); };
Results: module {
    Box: adt { value: int; }; # Copied position changed and returned through its original identity.
    compose: fn(box: ref Box, option: int, absent: string): (ref Box, array of chan of int, chan of string);
    buffered: fn(seed: string): chan of string;
    meeting: fn(seed: int): chan of int;
    retired: fn(ignored: int): int;
};
sys: Sys; # Borrowed system interface for fixture setup and polling.

include "named_movie_upload.b";


# EN: Verify transactional copy-back and distinct leased keys, including an all-nil result.
# RU: Сверить обратную запись и раздельные ключи аренд, включая результат только с nil.
compound(service: Results)
{
    box := ref Results->Box(12);
    (same, ports, text) := service->compose(box, 0, nil);
    if (box.value != 17 || same == nil || ports == nil || len ports != 3 ||
        ports[0] == nil || ports[0] != ports[1] || ports[2] != nil || text == nil) {
        raise "incorrect graph updates or channel aliases";
    }
    same.value = 41;
    if (box.value != 41 || <-ports[0] != 71 || <-text != "label") {
        raise "result did not reuse the original ref and its typed channel keys";
    }
    ports[1] <-= 72;
    if (<-ports[0] != 72) {
        raise "aliased result channels do not share their queue";
    }
    refused := 0;
    {
        (ignored, other, unused) := service->compose(box, 1, nil);
        if (ignored != nil || other != nil || unused != nil) {
            raise "unexpected valid bad result";
        }
    } exception {
    "unexpected valid bad result" =>
        raise "invalid composite result succeeded";
    * =>
        refused = 1;
    }
    if (!refused || box.value != 41) {
        raise "failed composite reply applied a ref update";
    }
    (same, ports, text) = service->compose(box, 2, nil);
    if (box.value != 46 || same != box || text != nil || ports == nil ||
        len ports != 1 || ports[0] != nil) {
        raise "all-nil channel result did not preserve the value fields";
    }
}


# EN: Confirm independent direction queues and select both sides of a rendezvous.
# RU: Проверить раздельные направления очереди и обе стороны rendezvous.
exchange(service: Results)
{
    fifo := service->buffered("hello");
    if (fifo == nil || <-fifo != "hello") {
        raise "buffered endpoint did not publish its seed";
    }
    fifo <-= "from-dis";
    if (<-fifo != "echo:from-dis") {
        raise "peer received its own send or the owner failed to reply";
    }
    fifo = nil;

    sync := service->meeting(5);
    if (sync == nil) {
        raise "rendezvous endpoint did not publish";
    }
    idle := chan of int;
    alt {
    sync <-= 42 =>
        ;
    wrong := <-idle =>
        raise sys->sprint("unexpected receive branch %d", wrong);
    }
    alt {
    answer := <-sync =>
        if (answer != 47) {
            raise "the owner lost the rendezvous value";
        }
    idle <-= -1 =>
        raise "unexpected send branch";
    }
    sync = nil;
    for (i := 0; i < 200; ++i) {
        if (service->retired(0) == 1) {
            return;
        }
        sys->sleep(10);
    }
    raise "closing the result did not retire its owner endpoint";
}


# EN: Load the proper provider through the ordinary mounted Dis interface.
# RU: Загрузить подходящий провайдер через штатный смонтированный интерфейс Dis.
init()
{
    sys = load Sys Sys->PATH;
    (movie, nil) := sys->stat("result-channels.swf");
    if (movie == 0) {
        start("result-channels.swf");
    }
    service: Results;
    for (i := 0; i < 200 && service == nil; ++i) {
        service = load Results "/peer/mounted/channel-results.dis";
        if (service == nil) {
            sys->sleep(10);
        }
    }
    if (service == nil) {
        raise sys->sprint("result-channel module did not load: %r");
    }
    compound(service);
    exchange(service);
    sys->print("PASS native result-channels RPC paired VMs\n");
}
