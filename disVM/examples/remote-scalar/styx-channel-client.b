# Copyright (c) 2026 bhgv
# EN: Load a remote library normally and pass live local Limbo channels.
# RU: Загрузить удалённую библиотеку обычным load и передать живые каналы Limbo.
implement Channelclient;
include "sys.m";
include "processctl.m";
include "chain-channel.m";

Channelclient: module { init: fn(); };


# EN: Advertise a channel inside its first message and verify identity across both directions.
# RU: Выдать канал внутри первого сообщения и проверить идентичность в обоих направлениях.
check_nested(remote: Chainchannel)
{
    input := chan[2] of Chainchannel->Relayed;
    output := chan[1] of Chainchannel->Relayed;
    known := chan of int;
    fresh := chan[1] of int;
    input <-= ("new", array[] of {fresh, nil, known}, known);
    input <-= ("alias", array[] of {fresh, fresh}, fresh);
    if (remote->nested(input, output, known) != fresh) {
        raise "nested channel was not returned as its original";
    }
    (text, ports, left) := <-output;
    if (text != "back" || len ports != 3 || ports[0] != fresh || ports[1] != nil ||
        ports[2] != known || left != fresh || <-fresh != 44) {
        raise "nested channel was not delivered with its original queue";
    }
    fresh <-= 65;
    (slot, value, chosen) := remote->choose(array[] of {nil, known, fresh, fresh});
    if ((slot != 2 && slot != 3) || value != 65 || len chosen != 4 || chosen[0] != nil ||
        chosen[1] != known || chosen[2] != fresh || chosen[3] != fresh) {
        raise "channel array changed its member index or identity";
    }
}


# EN: Pass an aggregate exceeding a single Styx message and validate the reconstructed result.
# RU: Передать составное значение больше сообщения Styx и проверить восстановленный итог.
check_typed(remote: Chainchannel)
{
    Item: import remote;
    input := chan[1] of Chainchannel->Payload;
    output := chan[1] of Chainchannel->Payload;
    wide := chan of big;
    tiny := chan[1] of byte;
    fraction := chan[1] of real;
    text := chan[1] of string;
    data := array[900] of int;
    for (i := 0; i < len data; i++) {
        data[i] = i*2;
    }
    item := ref Item(37);
    input <-= ("Юнибас", data, item, item);
    tiny <-= byte 251;
    fraction <-= 1.25;
    text <-= nil;
    next := chan of int;
    spawn typed_wide(output, wide, next);
    if (remote->typed(input, output, wide) != 1 || <-next != 1 ||
        data[899] != 1798 || item.number != 37 ||
        remote->scalars(tiny, fraction, text) != 1 || <-text != "✓") {
        raise "typed remote channels failed";
    }
}


# EN: Make the wide guard ready only after the provider has finished using the first guard.
# RU: Сделать широкую ветку готовой после завершения провайдером первой ветки.
typed_wide(output: chan of Chainchannel->Payload, wide: chan of big, done: chan of int)
{
    (text, data, first, second) := <-output;
    wide <-= big 9223372036854775807;
    done <-= text == "✓" && len data == 900 && data[899] == -18 &&
        first != nil && first == second && first.number == 91;
}


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
    buffered <-= 53;
    if (remote->exchange(6, input, (nil, input)) != 77 ||
        remote->exchange(7, buffered, (nil, buffered)) != 54 ||
        remote->exchange(7, input, (nil, input)) != -5) {
        raise "mixed fallback alt chose the wrong guard";
    }
    buffered <-= 61;
    if (remote->exchange(8, input, (buffered, input)) != 361 ||
        remote->exchange(9, input, (nil, input)) != 341 ||
        remote->exchange(10, input, (nil, input)) != -1923) {
        raise "remote alt array lost its member index or changed the fallback destination";
    }
    buffered <-= 17;
    if (remote->exchange(11, buffered, (nil, buffered)) != 117) {
        raise "mixed alt array missed its remote member";
    }
    check_typed(remote);
    check_nested(remote);
    returned := remote->channelresult(buffered, 0);
    if (returned != buffered || remote->channelresult(buffered, 1) != nil) {
        raise "returned channel lost caller identity";
    }
    returned <-= 71;
    if (<-buffered != 71) {
        raise "returned channel no longer uses the original queue";
    }
    rejected := 0;
    {
        remote->channelresult(buffered, 2);
    } exception {
        * => rejected = 1;
    }
    if (!rejected || remote->channelresult(buffered, 0) != buffered) {
        raise "provider channel result bypassed capability validation";
    }
    sys->print("PASS automatic Limbo channels over UniBus\n");
}
