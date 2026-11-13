# Copyright (c) 2026 bhgv
# EN: Actual callback results export live variables/functions and collect independently.
# RU: Actual callback results экспортируют live variables/functions и собираются независимо.
implement ProxyResultQuota;
include "sys.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyResultQuota: module { init: fn(); };
net: Proxynet; # Immutable initialized network adapter.


# EN/RU: Keep original connection broker resident until terminal Stop and join.
# Удерживать original connection broker до terminal Stop и join.
connect(input, output: ref Sys->FD, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 5000, ready);
    stopped <-= 1;
}


# EN: Remote export quota may fail after callback effects. Reclaiming one export
# permits a later independent call; Capacity never authorizes automatic replay.
# RU: Remote export quota может отказать после callback effects. Освобождение
# export допускает следующий independent call; Capacity не разрешает replay.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    stopped := chan[1] of int;
    spawn connect(sys->fildes(0), sys->fildes(1), stopped, ready);
    (status, endpoint, variable, function) := <-ready;
    if (status != 0) {
        raise "result quota greeting";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    pool := array[14] of Proxynet->Image;
    for (index := 0; index < len pool; index++) {
        (status, pool[index]) = net->fetch(endpoint, function, variable, variable, 2,
            index, Proxyhost->Variable, 10000);
        if (status != 0) {
            raise "result quota failed before configured owner limit";
        }
    }
    failed: Proxynet->Image;
    (status, failed) = net->fetch(endpoint, function, variable, variable, 2, -889, Proxyhost->Variable, 10000);
    if (status != Proxyhost->Capacity || failed.lease != nil) {
        raise "remote export quota did not refuse result ownership";
    }
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 11) {
        raise "remote result capacity hid or replayed completed callback effect";
    }
    (status, value) = net->call(endpoint, Proxyowner->Drop, pool[0], empty, empty, 0, 0, 10000);
    pool[0] = empty;
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    (status, failed) = net->fetch(endpoint, function, variable, variable, 2, -889, Proxyhost->Variable, 10000);
    if (status != 0) {
        raise "result quota did not recover after released export";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, failed, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 12) {
        raise "recovered result did not own its actual ref alias";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "result quota Stop did not collect outstanding exports";
    }
    sys->fprint(sys->fildes(2), "PASS remote result quota: completed effect preserved release recovery terminal roots\n");
}
