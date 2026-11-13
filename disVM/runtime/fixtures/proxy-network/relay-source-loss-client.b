# Copyright (c) 2026 bhgv
# EN: VM C imports GC-owned routes from B and invokes B with actual refs of A.
# RU: VM C импортирует GC-owned routes B и вызывает B с actual refs A.
implement ProxyRelaySourceLoss;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyRelaySourceLoss: module { init: fn(); };
net: Proxynet; # Immutable client connection interface.


# EN/RU: Keep the B connection alive until all returned route images are collected.
# Удержать B connection до collection returned route images.
connect(input, output: ref Sys->FD, done: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 33333, big 44444, 12000, ready);
    done <-= 1;
}


# EN/RU: Uncertain remote effects never authorize replay; independent B storage remains usable.
# Uncertain remote effects не разрешают replay; independent B storage остаётся usable.
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
        raise "forwarded fault greeting";
    }
    (imported, image) := net->importref(endpoint, function, 0, 10000);
    if (imported != 0) {
        raise "forwarded fault import";
    }
    value: int;
    (status, value) = net->routed(endpoint, function, image, image, 2, 1, 20000);
    if (status != Proxyowner->OwnerError && status != Proxyowner->Unknown) {
        raise "uncertain forwarded effect reported successful callback";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (status, value) = net->call(endpoint, Proxyowner->Read, local, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 0) {
        raise "source loss stopped unrelated B owner";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, image, empty, empty, 0, 0, 10000);
    if (status != Proxyowner->Unknown) {
        raise "closed forwarded source remained usable";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, local, empty, empty, 0, 0, 10000);
    if (status != Proxynet->Disconnected || <-stopped != 1) {
        raise "forwarded terminal connection replayed an access";
    }
    sys->fprint(sys->fildes(2), "PASS forwarding source loss: uncertain effect unrelated B owner no replay join\n");
}
