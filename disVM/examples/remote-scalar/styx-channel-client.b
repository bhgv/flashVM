# Copyright (c) 2026 bhgv
# EN: Load a remote library normally and pass live local Limbo channels.
# RU: Загрузить удалённую библиотеку обычным load и передать живые каналы Limbo.
implement Channelclient;
include "sys.m";
include "processctl.m";
include "chain-channel.m";

Channelclient: module { init: fn(); };


# EN: Exchange values through an ordinary local alt and report the observed result.
# RU: Обменяться значениями через обычный локальный alt и сообщить принятый результат.
local_peer(input, output, done: chan of int)
{
    alt {
        input <-= 42 =>
            done <-= <-output;
    }
}


# EN: Publish this worker's PID, then let its owner cancel the waiting call.
# RU: Сообщить PID исполнителя, затем дать владельцу отменить ожидающий вызов.
will_cancel(remote: Chainchannel, control: Processctl, parked, started, done: chan of int)
{
    started <-= control->pid();
    input := chan of int;
    failed := 0;
    {
        remote->exchange(2, input, (parked, input));
    } exception {
        * => failed = 1;
    }
    done <-= failed;
}


# EN: Check automatic argument binding, rendezvous, buffering and cancellation.
# RU: Проверить автоматическую привязку аргументов, обмен, буферизацию и отмену.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    remote := load Chainchannel "/transport/mounted/channel.dis";
    if (remote == nil || remote->exchange(3, nil, (nil, nil)) != 1) {
        raise sys->sprint("cannot load typed channel example: %r");
    }
    input := chan of int;
    output := chan of int;
    done := chan of int;
    spawn local_peer(input, output, done);
    if (remote->exchange(0, input, (output, input)) != 25 || <-done != 59) {
        raise "remote channel rendezvous failed";
    }
    buffered := chan[1] of int;
    if (remote->exchange(1, nil, (buffered, nil)) != 23 || <-buffered != -17) {
        raise "remote channel lost the caller's buffer";
    }
    started := chan of int;
    spawn will_cancel(remote, control, buffered, started, done);
    pid := <-started;
    if (<-buffered != -23 || control->cancelwait(pid) != Processctl->Cancelled || <-done != 1) {
        raise "remote channel cancellation failed";
    }
    if (remote->exchange(1, nil, (buffered, nil)) != 23 || <-buffered != -17) {
        raise "cancelled channel blocked the next call";
    }
    buffered <-= 53;
    if (remote->exchange(4, input, (buffered, input)) != 52 ||
        remote->exchange(5, nil, (output, nil)) != 37 ||
        remote->exchange(5, nil, (buffered, nil)) != 31 || <-buffered != -29) {
        raise "remote alt chose the wrong guard";
    }
    sys->print("PASS automatic Limbo channels over UniBus\n");
}
