# Copyright (c) 2026 bhgv
# EN: Publish a typed channel and send a fresh channel back through a reverse grant.
# RU: Выдать типизированный канал и отправить новый канал по встречной выдаче.
implement Channelrecursive;
include "chain-channel-recursive.m";


# EN: Return the first channel and an independent completion channel to the caller.
# RU: Вернуть вызывающему первый канал и отдельный канал завершения.
open(): (chan of chan of chan of chan of int, chan of int)
{
    input := chan of chan of chan of chan of int;
    result := chan of int;
    spawn exchange(input, result);
    return (input, result);
}


# EN: Receive a reverse channel, grant a new module channel and use its client leaf.
# RU: Принять встречный канал, выдать новый канал модуля и использовать канал клиента.
exchange(input: chan of chan of chan of chan of int, result: chan of int)
{
    reverse := <-input;
    outgoing := chan of chan of int;
    reverse <-= outgoing;
    reverse <-= outgoing;
    if (<-reverse != outgoing) {
        raise "echoed module grant changed identity";
    }
    peer := <-outgoing;
    peer <-= 43;
    value := <-peer;
    result <-= value;
}
