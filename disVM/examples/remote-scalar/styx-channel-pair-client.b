# Copyright (c) 2026 bhgv
# EN: Lend unbuffered channels while this resident only waits for the function reply.
# RU: Выдать небуферизованные каналы и ждать только ответа на вызов функции.
implement Channelpairclient;
include "sys.m";
include "channel-pair.m";

Channelpairclient: module { init: fn(); };


# EN: Verify both cross-resident matches with no second caller-side thread.
# RU: Проверить оба сопоставления на провайдере без второго потока на клиенте.
init()
{
    sys := load Sys Sys->PATH;
    remote := load Channelpair "/transport/mounted/pair.dis";
    wire := chan of int;
    absent := chan of string;
    payload := chan of Channelpair->Payload;
    unused := chan of int;
    if (remote == nil || remote->integer(wire, absent, 0) != 1 ||
        remote->integer(wire, absent, 1) != 1 ||
        remote->managed(payload, unused) != 1) {
        raise "same-bus proxy rendezvous failed";
    }
    sys->print("PASS same-bus proxy channel rendezvous\n");
}
