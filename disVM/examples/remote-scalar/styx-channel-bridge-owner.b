# Copyright (c) 2026 bhgv
# EN: Act as the middle DisVM between a caller and a mounted channel owner.
# RU: Служить промежуточной DisVM между клиентом и владельцем каналов.
implement Channelbridge;
include "chain-channel-bridge.m";
include "chain-channel-lifetime.m";


# EN: Load C and grant the caller's channel while A's invocation stays alive.
# RU: Загрузить C и выдать канал вызывающего, пока действует вызов от A.
roundtrip(port: chan of int, value: int): (int, chan of int)
{
    owner := load Chainchannellifetime "/dependency/mounted/channel-lifetime.dis";
    if (owner == nil) {
        raise "bridge could not load the channel owner";
    }
    return owner->relay(port, value);
}


# EN: Return C's channels under a new lease at B.
# RU: Вернуть каналы C под новой арендой на B.
worker(start: int): (chan of int, chan of int)
{
    owner := load Chainchannellifetime "/dependency/mounted/channel-lifetime.dis";
    if (owner == nil) {
        raise "bridge could not load the channel owner";
    }
    return owner->worker(start);
}


# EN: Keep both call fids open until a value arrives or the first caller cancels.
# RU: Удерживать оба fid вызовов до поступления значения или отмены клиентом.
waitvalue(port, entered: chan of int): int
{
    owner := load Chainchannellifetime "/dependency/mounted/channel-lifetime.dis";
    if (owner == nil) {
        raise "bridge could not load the waiting export";
    }
    return owner->waitvalue(port, entered);
}
