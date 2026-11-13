# Copyright (c) 2026 bhgv
# EN: VM C imports GC-owned routes from B and invokes B with actual refs of A.
# RU: VM C импортирует GC-owned routes B и вызывает B с actual refs A.
implement ProxyRelayClient;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyRelayClient: module { init: fn(); };
net: Proxynet; # Immutable client connection interface.


# EN/RU: Keep the B connection alive until all returned route images are collected.
# Удержать B connection до collection returned route images.
connect(input, output: ref Sys->FD, done: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 33333, big 44444, 12000, ready);
    done <-= 1;
}


# EN/RU: Each remote call supplies forwarded ref aliases, never a local B Cell.
# Каждый remote call передаёт forwarded ref aliases, не local B Cell.
client(endpoint: ref Proxynet->Endpoint, function, first, second: Proxynet->Image, done: chan of int)
{
    for (round := 0; round < 2; round++) {
        (status, value) := net->routed(endpoint, function, first, second, 2, 1, 20000);
        if (status != 0 || value < 12) {
            raise "third VM routed invocation lost original refs";
        }
    }
    done <-= 1;
}


# EN/RU: Exercise import authority, actual alias storage, plain-call refusal and transitive GC.
# Проверить import authority, actual alias storage, отказ plain call и transitive GC.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    stopped := chan[1] of int;
    spawn connect(sys->fildes(0), sys->fildes(1), stopped, ready);
    (status, endpoint, local, function) := <-ready;
    if (status != 0) {
        raise "third VM B greeting";
    }
    (first_status, first) := net->importref(endpoint, function, 0, 10000);
    (second_status, second) := net->importref(endpoint, function, 0, 10000);
    if (first_status != 0 || second_status != 0 || first.owner.slot == second.owner.slot) {
        raise "third VM route import ownership";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Read, first, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 10) {
        raise "forwarded variable became a registry stub snapshot";
    }
    (status, value) = net->call(endpoint, Proxyowner->Invoke, function, first, first, 2, 1, 10000);
    if (status != 3) {
        raise "ordinary callback received a forwarding stub as local Cell";
    }
    (status, value) = net->routed(endpoint, function, local, local, 2, 999, 10000);
    if (status != Proxynet->Stale) {
        raise "unregistered local Cell accepted as foreign route";
    }
    (bad_status, bad) := net->importref(endpoint, function, 1, 10000);
    if (bad_status != 3 || bad.lease != nil) {
        raise "unregistered route slot became accessible";
    }
    done := chan of int;
    for (index := 0; index < 8; index++) {
        spawn client(endpoint, function, first, second, done);
    }
    for (index = 0; index < 8; index++) {
        <-done;
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, second, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 42) {
        raise "third VM routed updates did not reach original A storage";
    }
    (clone_status, retained) := net->clone(endpoint, first, 10000);
    if (clone_status != 0) {
        raise "forwarded route clone";
    }
    (status, value) = net->call(endpoint, Proxyowner->Drop, first, empty, empty, 0, 0, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Read, retained, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 42) {
        raise "forwarded original Drop revoked retained route owner";
    }
    retained = empty;
    second = empty;
    collected := 0;
    for (attempt := 0; attempt < 1000 && collected < 2; attempt++) {
        (status, collected) = net->collected(endpoint, 10000);
        sys->sleep(1);
    }
    if (collected < 2) {
        raise "third VM image GC did not release forwarded route owners";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, local, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 0) {
        raise "forwarded refs overwrote B local variable";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "third VM B Stop cleanup";
    }
    sys->fprint(sys->fildes(2), "PASS three VM routes: imports live refs 8 callers clone Drop transitive GC\n");
}
