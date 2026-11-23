# Copyright (c) 2026 bhgv
# EN: Lend real Dis channels to FlashVM from the dedicated native runtime.
# RU: Передача настоящих каналов Dis во FlashVM из выделенного нативного рантайма.
implement Client;
include "sys.m";
include "channel_incoming.m";

Client: module { init: fn(); };


# EN: Send one item on an unbuffered port from another native Dis owner.
# RU: Отправить один элемент в небуферизованный port из другого нативного владельца.
offer(port: chan of int)
{
    port <-= -19;
}


# EN: Check separate leased results, even when calls reuse the same borrowed channel.
# RU: Проверить независимые аренды при повторной передаче одного аргумента-канала.
check_results(api: Incoming, port: chan of int)
{
    port <-= 107;
    one := api->make(port);
    port <-= -4;
    two := api->make(port);
    if (one == nil || two == nil || one == two || <-one != 107 || <-two != -4) {
        raise "result channels lost independent seeds";
    }
    one <-= 81;
    alt {
        value := <-one =>
            if (value != 81) {
                raise "leased FIFO returned the wrong value";
            }
        * => raise "leased FIFO is not ready after return";
    }
    wrapper := api->sync(port);
    empty := <-wrapper;
    wrapper = nil;
    if (empty == nil) {
        raise "missing result rendezvous";
    }
    spawn offer(empty);
    if (<-empty != -19) {
        raise "leased rendezvous failed after the call";
    }
    rejected := 0;
    port <-= 101;
    {
        api->badseed(port);
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected) {
        raise "rendezvous accepted an initial element";
    }
    alt {
        queued := <-port => raise "failed result rolled back the input receive";
        * => ;
    }
    one = nil;
    two <-= 34;
    if (<-two != 34) {
        raise "retiring a result affected another issuance";
    }
}


# EN: Lend a message channel, then check the returned aliases and the nested send.
# RU: Передать канал сообщений, затем сверить возвращённые алиасы и вложенную отправку.
check_nested(api: Incoming)
{
    outer := chan[2] of array of chan of int;
    inner := chan[1] of int;
    inner <-= 41;
    outer <-= array[] of {inner, inner, nil};
    outer <-= array[] of {inner, inner, nil};
    if (api->nested(outer) != 41) {
        raise "nested port receive returned the wrong value";
    }
    aliases := <-outer;
    if (len aliases != 3 || aliases[0] != inner || aliases[1] != inner || aliases[2] != nil || <-inner != 42) {
        raise "returned message did not use the original nested port";
    }
}


# EN: Mount the compiled interface, then exercise channel grants across several calls.
# RU: Загрузить компилированный интерфейс и проверить полномочия в нескольких вызовах.
init()
{
    sys := load Sys Sys->PATH;
    api := load Incoming "/rpc/mounted/flash-incoming.dis";
    if (api == nil) {
        raise sys->sprint("cannot load FlashVM arguments: %r");
    }
    ints := chan[1] of int;
    text := chan[1] of string;
    ints <-= 41;
    text <-= "a\0б";
    if (api->exchange(array[] of {ints, ints, nil}, text) != 41) {
        raise "wrong copied result";
    }
    if (<-ints != 42 || <-text != "a\0б!") {
        raise "lost reverse channel modifications";
    }
    pending := chan of int;
    spawn offer(pending);
    if (api->relay(pending) != -19) {
        raise "AVM1 could not receive a native rendezvous";
    }
    rejected := 0;
    {
        api->reject(ints);
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected || <-ints != 99) {
        raise "lost callback exception or its channel effect";
    }
    rejected = 0;
    {
        api->relay(ints);
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected) {
        raise "empty blocking receive returned before timing out";
    }
    check_results(api, ints);
    check_nested(api);
    api->finish(ints);
    ints <-= 11;
    if (<-ints != 11) {
        raise "closing the borrowed object changed the native channel";
    }
    sys->print("PASS DisVM loaded FlashVM channel argument handlers\n");
}
