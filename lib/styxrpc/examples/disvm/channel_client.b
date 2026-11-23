# Copyright (c) 2026 bhgv
# EN: Pass local channels to a C device and exchange strings through ordinary Limbo.
# RU: Передача локальных каналов C-устройству и обмен строками обычными операциями Limbo.
implement Client;
include "sys.m";

Client: module { init: fn(); };

Bridge: module {
    exchange: fn(input: chan of string, output: chan of string, idle: chan of string): int;
    port: fn(): chan of string;
};


# EN: Use an unbuffered send followed by selection on the receiving side.
# RU: Использовать синхронную отправку и затем выбор на стороне приёма.
worker(input, output, idle, done: chan of string)
{
    input <-= "from-dis";
    alt {
    text := <-output =>
        done <-= text;
    <-idle =>
        done <-= "wrong guard";
    }
}


# EN: Load an independent C provider and keep the caller's channels live through the call.
# RU: Загрузить независимого C-провайдера и удержать каналы клиента до возврата.
init()
{
    sys := load Sys Sys->PATH;
    bridge := load Bridge "/device/mounted/bridge.dis";
    if (bridge == nil) {
        raise sys->sprint("cannot load C channel bridge: %r");
    }
    input := chan of string;
    output := chan of string;
    idle := chan of string;
    done := chan of string;
    spawn worker(input, output, idle, done);
    if (bridge->exchange(input, output, idle) != 42 || <-done != "from-c") {
        raise "wrong remote channel exchange";
    }
    port := bridge->port();
    if (port == nil) {
        raise "missing C-owned channel";
    }
    port <-= "held-by-c";
    alt {
    message := <-port =>
        if (message != "held-by-c") {
            raise "C-owned buffer returned wrong contents";
        }
    * =>
        raise "C-owned channel lost a ready guard";
    }
    sys->print("PASS Limbo/C live channels over Styx\n");
}
