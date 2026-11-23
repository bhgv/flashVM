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
        api->timeout(ints);
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected) {
        raise "empty blocking receive returned before timing out";
    }
    api->finish(ints);
    ints <-= 11;
    if (<-ints != 11) {
        raise "closing the borrowed object changed the native channel";
    }
    sys->print("PASS DisVM loaded FlashVM channel argument handlers\n");
}
