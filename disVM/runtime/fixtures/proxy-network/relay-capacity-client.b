# Copyright (c) 2026 bhgv
# EN: VM C imports GC-owned routes from B and invokes B with actual refs of A.
# RU: VM C импортирует GC-owned routes B и вызывает B с actual refs A.
implement ProxyRelayCapacity;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyRelayCapacity: module { init: fn(); };
net: Proxynet; # Immutable client connection interface.


# EN/RU: Keep the B connection alive until all returned route images are collected.
# Удержать B connection до collection returned route images.
connect(input, output: ref Sys->FD, done: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 33333, big 44444, 12000, ready);
    done <-= 1;
}


# EN/RU: Native reservation refusal precedes all wire imports and keeps B usable.
# Native reservation refusal предшествует wire imports и сохраняет B usable.
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
        raise "forwarded reserve greeting";
    }
    (imported, image) := net->importref(endpoint, function, 0, 10000);
    if (imported != Proxyhost->Capacity || image.lease != nil) {
        raise "forwarded reserve refusal changed output or executed import";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Read, local, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 0) {
        raise "forwarded reserve refusal closed live connection";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "forwarded reserve cleanup";
    }
    sys->fprint(sys->fildes(2), "PASS forwarded reserve: no wire import live output preserved join\n");
}
