# Copyright (c) 2026 bhgv
# EN: Original function owner in this VM uses live proxy refs to another actual VM.
# RU: Original function owner этой VM работает с live proxy refs другой actual VM.
implement ProxyRoutedFailure;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
ProxyRoutedFailure: module { init: fn(); };
net: Proxynet; # Immutable original remote connection interface.
proxy: Proxyowner; # Local original function owner, on another permanent Prog.
route: Proxyroute; # Independent source-image owners behind typed ref actors.
access: Proxyref; # Variable-only primitive access, no nested function dispatch.


# EN/RU: Original connection stays resident until Stop; signal after its loop exits.
# Original connection живёт до Stop; сообщить завершение его loop.
connect(input, output: ref Sys->FD, done: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 10000, ready);
    done <-= 1;
}


# EN/RU: Announce this original function owner's PID before serving captured state.
# Сообщить PID original function owner до обслуживания captured state.
own(handler: Proxyhandler, cell, capture: ref Proxyhost->Cell, started, done: chan of int,
    ready: chan of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle))
{
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    proxy->serve(handler, cell, capture, ready);
    done <-= 1;
}


# EN/RU: Ref actor owns a distinct remote clone until Close/last ref GC/expiry.
# Ref actor владеет distinct remote clone до Close/last ref GC/expiry.
attach(endpoint: ref Proxynet->Endpoint, image: Proxynet->Image, done: chan of int,
       ready: chan of (int, ref Proxyref->Ref))
{
    route->start(endpoint, image, 5000, ready);
    done <-= 1;
}


# EN/RU: Buffered outcome lets caller retirement never strand an original owner.
# Buffered outcome не оставляет original owner без получателя после retirement caller.
caller(endpoint: ref Proxyowner->Endpoint, function: Proxyowner->Handle,
       source: ref Proxyref->Ref, done: chan of int)
{
    (status, unused) := proxy->routed(endpoint, function, source, source, 2, 1, 5000);
    done <-= status;
}


# EN/RU: Two callbacks survive uncertain access effects; unrelated local owner remains usable.
# Два callbacks переживают uncertain access effects; unrelated local owner остаётся usable.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    proxy = load Proxyowner Proxyowner->PATH;
    route = load Proxyroute Proxyroute->PATH;
    access = load Proxyref Proxyref->PATH;
    net->init();
    proxy->init();
    route->init();
    access->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    network_done := chan[1] of int;
    spawn connect(sys->fildes(0), sys->fildes(1), network_done, ready);
    (status, endpoint, variable, remote_function) := <-ready;
    if (status != 0) {
        raise "routed fault greeting";
    }
    refs_ready := chan[1] of (int, ref Proxyref->Ref);
    actor_done := chan[1] of int;
    spawn attach(endpoint, variable, actor_done, refs_ready);
    (attached, source) := <-refs_ready;
    if (attached != 0) {
        raise "routed fault source attachment";
    }
    started := chan[1] of int;
    owner_done := chan[2] of int;
    owner_ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    first_cell := ref Proxyhost->Cell(0);
    first_capture := ref Proxyhost->Cell(0);
    handler := load Proxyhandler "handler.dis";
    spawn own(handler, first_cell, first_capture, started, owner_done, owner_ready);
    original := <-started;
    (first, first_variable, first_function) := <-owner_ready;
    second_cell := ref Proxyhost->Cell(0);
    second_capture := ref Proxyhost->Cell(0);
    spawn own(handler, second_cell, second_capture, started, owner_done, owner_ready);
    other_original := <-started;
    (second, second_variable, second_function) := <-owner_ready;
    done := chan[2] of int;
    spawn caller(first, first_function, source, done);
    spawn caller(second, second_function, source, done);
    for (index := 0; index < 2; index++) {
        status = <-done;
        if (status != Proxyowner->OwnerError && status != Proxyowner->Unknown) {
            raise "uncertain routed effect was reported as a successful callback";
        }
    }
    if (<-actor_done != 1 || <-network_done != 1) {
        raise "routed fault actors did not finish";
    }
    value: int;
    (status, value) = access->call(source, Proxyref->Write, 999, sys->millisec() + 1000);
    if (status != Proxyref->Disconnected) {
        raise "routed fault ref attempted another access";
    }
    absent := Proxyowner->Handle(big 0, 0, big 0);
    (status, value) = proxy->call(first, Proxyowner->Read, first_variable, absent, absent, 0, 0, 10000);
    if (status != 0 || value != 0 || first_capture.value != original || second_capture.value != other_original) {
        raise "source disconnect stopped unrelated original owner or migrated capture";
    }
    (status, value) = proxy->call(first, Proxyowner->Stop, absent, absent, absent, 0, 0, 10000);
    (status, value) = proxy->call(second, Proxyowner->Stop, absent, absent, absent, 0, 0, 10000);
    if (status != 0 || <-owner_done != 1 || <-owner_done != 1) {
        raise "routed fault native function owners did not retire";
    }
    sys->fprint(sys->fildes(2), "PASS routed disconnect: uncertain effect no replay two callbacks unrelated owner join\n");
}
