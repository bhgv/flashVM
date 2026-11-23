# Copyright (c) 2026 bhgv
# EN: Check A → B → C channel forwarding across two independent TCP sessions.
# RU: Проверка пересылки каналов A → B → C через две независимые сессии TCP.
implement Bridgeclient;
include "sys.m";
include "processctl.m";
include "chain-channel-bridge.m";

Bridgeclient: module
{
    init: fn();
};


# EN: Wait through B and C. Report the local PID and whether cancellation unwound the call.
# RU: Ждать через B и C. Сообщить свой PID и дошла ли отмена до вызова.
cancelled_call(bridge: Channelbridge, ctl: Processctl, port, entered, started, done: chan of int)
{
    started <-= ctl->pid();
    cancelled := 0;
    {
        value := bridge->waitvalue(port, entered);
    } exception {
        * => cancelled = 1;
    }
    done <-= cancelled;
}


# EN: Pass a live local queue forward and use a channel created on C after B returns.
# RU: Передать локальную очередь и использовать канал C после возврата B.
init()
{
    sys := load Sys Sys->PATH;
    bridge := load Channelbridge "/transport/mounted/bridge.dis";
    if (bridge == nil) {
        raise sys->sprint("cannot load channel bridge: %r");
    }
    local := chan[1] of int;
    local <-= 17;
    (previous, alias) := bridge->roundtrip(local, 29);
    if (previous != 17 || alias != local || <-local != 29) {
        raise "two-hop argument lost its value or original identity";
    }
    ctl := load Processctl Processctl->PATH;
    waiting := chan of int;
    entered := chan of int;
    started := chan of int;
    done := chan of int;
    spawn cancelled_call(bridge, ctl, waiting, entered, started, done);
    pid := <-started;
    if (<-entered != 1 || ctl->cancelwait(pid) != Processctl->Cancelled || <-done != 1) {
        raise "cancellation did not unwind the two-hop call";
    }
    (input, output) := bridge->worker(100);
    bridge = nil;
    input <-= 13;
    if (<-output != 113) {
        raise "two-hop channel result did not reach C's original queue";
    }
    sys->print("PASS live channels across three DisVMs\n");
}
