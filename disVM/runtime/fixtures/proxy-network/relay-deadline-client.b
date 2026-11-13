# Copyright (c) 2026 bhgv
# EN: VM C imports GC-owned routes from B and invokes B with actual refs of A.
# RU: VM C импортирует GC-owned routes B и вызывает B с actual refs A.
implement ProxyRelayDeadline;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
ProxyRelayDeadline: module { init: fn(); };
net: Proxynet; # Immutable client connection interface.


# EN/RU: Keep the B connection alive until all returned route images are collected.
# Удержать B connection до collection returned route images.
connect(input, output: ref Sys->FD, done: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 33333, big 44444, 12000, ready);
    done <-= 1;
}


# EN/RU: Expired startup offers no Retain; the source connection remains usable.
# Expired startup не передаёт Retain; source connection остаётся usable.
attach(source: ref Proxynet->Endpoint, image: Proxynet->Image, ready: chan of (int, ref Proxyref->Ref), done: chan of int)
{
    sys := load Sys Sys->PATH;
    route := load Proxyroute Proxyroute->PATH;
    route->init();
    route->startuntil(source, image, 5000, sys->millisec() - 1, ready);
    done <-= 1;
}


# EN/RU: Require explicit NotSent and nil output, then read and Stop without lost ownership.
# Требовать explicit NotSent и nil output, затем Read/Stop без потери ownership.
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
        raise "forwarded deadline greeting";
    }
    attached := chan[1] of (int, ref Proxyref->Ref);
    done := chan[1] of int;
    spawn attach(endpoint, local, attached, done);
    (created, image) := <-attached;
    if (created != Proxyref->NotSent || image != nil || <-done != 1) {
        raise "expired startup renewed its source deadline";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Read, local, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 0) {
        raise "expired startup damaged live source";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "expired startup did not join";
    }
    sys->fprint(sys->fildes(2), "PASS expired forwarding startup: no Retain nil output source alive join\n");
}
