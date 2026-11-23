# Copyright (c) 2026 bhgv
# EN: Load FlashVM's channel module and operate through ordinary Limbo syntax.
# RU: Загрузить модуль каналов FlashVM и использовать штатные операции Limbo.
implement Client;
include "sys.m";
include "channel_service.m";

Client: module { init: fn(); };


# EN: Offer a rendezvous from a second native Dis owner.
# RU: Предложить синхронную передачу со второго нативного владельца Dis.
send_int(port: chan of int, done: chan of int)
{
    port <-= -19;
    done <-= 1;
}


# EN: Use distinct call leases and confirm typed FIFO, guarded receive and rendezvous.
# RU: Проверить разные аренды, типизированный FIFO, выбор приёма и rendezvous.
init()
{
    sys := load Sys Sys->PATH;
    api := load Channelservice "/rpc/mounted/flash-channels.dis";
    if (api == nil) {
        raise sys->sprint("cannot load FlashVM channel exports: %r");
    }
    one := api->port("a\0б");
    two := api->port("second");
    if (one == nil || two == nil || one == two) {
        raise "distinct calls did not create distinct leases";
    }
    alt {
        first := <-one =>
            if (first != "a\0б") {
                raise "FlashVM string seed lost counted bytes";
            }
        * => raise "seeded FIFO was idle";
    }
    alt {
        spurious := <-one => raise "drained FIFO was still ready";
        * => ;
    }
    if (<-two != "second") {
        raise "another call lost its queue";
    }
    one <-= "reply";
    if (<-one != "reply") {
        raise "FlashVM FIFO changed the client value";
    }
    rejected := 0;
    {
        api->badseed("invalid int element");
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected) {
        raise "invalid initializer was accepted";
    }
    pending := api->rendezvous();
    done := chan[1] of int;
    if (pending == nil) {
        raise "missing rendezvous channel";
    }
    spawn send_int(pending, done);
    if (<-pending != -19 || <-done != 1) {
        raise "FlashVM did not pair the two guarded operations";
    }
    sys->print("PASS DisVM loaded FlashVM live channels\n");
}
