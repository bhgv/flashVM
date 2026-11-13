# Copyright (c) 2026 bhgv
# EN: Actual callback results export live variables/functions and collect independently.
# RU: Actual callback results экспортируют live variables/functions и собираются независимо.
implement ProxyResultClient;
include "sys.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyResultClient: module { init: fn(); };
net: Proxynet; # Immutable initialized network adapter.


# EN/RU: Keep original connection broker resident until terminal Stop and join.
# Удерживать original connection broker до terminal Stop и join.
connect(input, output: ref Sys->FD, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 5000, ready);
    stopped <-= 1;
}


# EN/RU: Concurrent result ownership, original callback/ref aliases and explicit release.
# Конкурентное result ownership, original callback/ref aliases и explicit release.
client(endpoint: ref Proxynet->Endpoint, variable, function: Proxynet->Image,
       index: int, done: chan of int)
{
    (status, result) := net->fetch(endpoint, function, variable, variable, 2,
        100 + index, Proxyhost->Variable, 10000);
    if (status != 0) {
        raise "concurrent callback variable result";
    }
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Invoke, function, result, result, 2, 1, 10000);
    if (status != 0 || value != 102 + index) {
        raise "returned variable is not a live original ref";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (status, value) = net->call(endpoint, Proxyowner->Drop, result, empty, empty, 0, 0, 10000);
    if (status != 0) {
        raise "returned variable explicit release";
    }
    done <-= 1;
}


# EN/RU: New export roots must outlive originals and become collectible before Stop.
# New export roots должны пережить originals и собраться до Stop.
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
        raise "result client greeting";
    }
    result, alias, callable, failed: Proxynet->Image;
    value: int;
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (status, result) = net->fetch(endpoint, function, variable, variable, 2, 31, Proxyhost->Variable, 10000);
    if (status != 0) {
        raise "new variable result export";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, result, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 31) {
        raise "new variable result value";
    }
    (status, alias) = net->fetch(endpoint, function, variable, variable, 2, -888, Proxyhost->Variable, 10000);
    if (status != 0 || alias.owner.slot == variable.owner.slot) {
        raise "returned alias lacks independent export ownership";
    }
    (status, value) = net->call(endpoint, Proxyowner->Write, alias, empty, empty, 0, 19, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 19) {
        raise "returned ref alias became a copied snapshot";
    }
    (status, callable) = net->fetch(endpoint, function, variable, variable, 2, 0, Proxyhost->Function, 10000);
    if (status != 0) {
        raise "new function result export";
    }
    (status, value) = net->call(endpoint, Proxyowner->Drop, function, empty, empty, 0, 0, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Invoke, callable, result, result, 2, 2, 10000);
    if (status != 0 || value != 35) {
        raise "returned function lost original execution/capture or ref result";
    }
    done := chan of int;
    for (index := 0; index < 8; index++) {
        spawn client(endpoint, alias, callable, index, done);
    }
    for (index = 0; index < 8; index++) {
        <-done;
    }
    (status, failed) = net->fetch(endpoint, callable, alias, alias, 2, -1, Proxyhost->Variable, 10000);
    if (status != Proxyowner->OwnerError || failed.lease != nil) {
        raise "failed callback published a result image";
    }
    (status, failed) = net->fetch(endpoint, callable, alias, alias, 2, -2, Proxyhost->Variable, 10000);
    if (status == 0 || failed.lease != nil) {
        raise "nil result became an export";
    }
    (status, value) = net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Read, alias, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 20) {
        raise "returned alias lost root after original Drop or callback partial effect";
    }
    result = empty;
    alias = empty;
    callable = empty;
    collected := 0;
    for (attempt := 0; attempt < 1000 && collected < 3; attempt++) {
        (status, collected) = net->collected(endpoint, 10000);
        sys->sleep(1);
    }
    if (collected < 3) {
        raise "returned images were not GC released";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "result client terminal cleanup";
    }
    sys->fprint(sys->fildes(2), "PASS callback result images: new variables functions aliases 8 clients errors GC\n");
}
