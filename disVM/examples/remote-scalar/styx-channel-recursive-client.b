# Copyright (c) 2026 bhgv
# EN: Verify repeated nested grants, aliases and retirement on one Styx connection.
# RU: Проверить повторные вложенные выдачи, идентичность и освобождение в одной сессии Styx.
implement Recursiveclient;
include "sys.m";
include "chain-channel-recursive.m";

Recursiveclient: module
{
    init: fn();
};


# EN: Exchange channels in alternating directions and release all roots at return.
# RU: Обменяться каналами в чередующихся направлениях и снять ссылки при выходе.
exchange(api: Channelrecursive, expected: int)
{
    (input, result) := api->open();
    reverse := chan of chan of chan of int;
    input <-= reverse;
    outgoing := <-reverse;
    alias := <-reverse;
    if (outgoing != alias) {
        raise "repeated module grant changed proxy identity";
    }
    reverse <-= alias;
    peer := chan of int;
    alias <-= peer;
    if (<-peer != 43) {
        raise "second reverse grant missed original queue";
    }
    peer <-= expected;
    if (<-result != expected) {
        raise "recursive channel exchange lost its return value";
    }
}


# EN: Load the module once and recycle beyond the module and reverse-session limits.
# RU: Загрузить модуль один раз и пройти за лимиты модуля и встречной сессии.
init()
{
    sys := load Sys Sys->PATH;
    api := load Channelrecursive "/transport/mounted/channel-lifetime.dis";
    if (api == nil) {
        raise sys->sprint("cannot load recursive channel fixture: %r");
    }
    for (round := 0; round < 132; round++) {
        exchange(api, 100+round);
    }
    sys->print("PASS nested grants across both UniBus directions\n");
}
