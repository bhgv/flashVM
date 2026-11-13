# Copyright (c) 2026 bhgv
# EN: Original function owner in this VM uses live proxy refs to another actual VM.
# RU: Original function owner этой VM работает с live proxy refs другой actual VM.
implement ProxyRoutedClient;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
ProxyRoutedClient: module { init: fn(); };


# EN/RU: An unreachable wrapper cycle must not keep the access actor's clone alive.
# Недостижимый wrapper cycle не должен сохранять clone access actor.
Cycle: adt
{
    next: ref Cycle; # Local unreachable cycle, requiring concurrent Cycles collection.
    source: ref Proxyref->Ref; # Last wrapper owner with an outgoing edge to actor mailbox.
};
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


# EN/RU: Each original-owner callback mutates both foreign refs using one shared deadline.
# Каждый original-owner callback меняет оба foreign refs с одним общим deadline.
client(endpoint: ref Proxyowner->Endpoint, function: Proxyowner->Handle,
       left, right: ref Proxyref->Ref, done: chan of int)
{
    for (round := 0; round < 2; round++) {
        (status, value) := proxy->routed(endpoint, function, left, right, 2, 1, 20000);
        if (status != 0 || value < 12) {
            raise "routed original-owner callback lost live aliases";
        }
    }
    done <-= 1;
}


# EN/RU: Distinct ref actors alias one actual remote Cell; original Drop cannot revoke them.
# Distinct ref actors ссылаются на одну actual remote Cell; original Drop их не отзывает.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
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
        raise "routed remote greeting";
    }
    refs_ready := chan[1] of (int, ref Proxyref->Ref);
    first_done := chan[1] of int;
    second_done := chan[1] of int;
    spawn attach(endpoint, variable, first_done, refs_ready);
    (first_status, left) := <-refs_ready;
    spawn attach(endpoint, variable, second_done, refs_ready);
    (second_status, right) := <-refs_ready;
    if (first_status != 0 || second_status != 0 || left == nil || right == nil) {
        raise "routed ref ownership attachment";
    }
    forged := ref Proxyref->Ref(left.requests, left.state, right.lease);
    (forged_status, forged_value) := access->call(forged, Proxyref->Add, 999, sys->millisec() + 10000);
    if (forged_status != Proxyref->Stale) {
        raise "routed ref accepted another actor token";
    }
    forged = nil;
    (witness_status, witness) := net->clone(endpoint, variable, 10000);
    if (witness_status != 0) {
        raise "routed source witness Retain";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Drop, remote_function, empty, empty, 0, 0, 10000);
    started := chan[1] of int;
    owner_done := chan[1] of int;
    owner_ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    local := ref Proxyhost->Cell(0);
    capture := ref Proxyhost->Cell(0);
    handler := load Proxyhandler "handler.dis";
    spawn own(handler, local, capture, started, owner_done, owner_ready);
    original := <-started;
    (function_owner, unused, function) := <-owner_ready;
    second_started := chan[1] of int;
    other_done := chan[1] of int;
    other_ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    other_cell := ref Proxyhost->Cell(0);
    other_capture := ref Proxyhost->Cell(0);
    spawn own(handler, other_cell, other_capture, second_started, other_done, other_ready);
    other_original := <-second_started;
    (other_owner, other_unused, other_function) := <-other_ready;
    done := chan of int;
    for (index := 0; index < 8; index++) {
        if (index % 2) {
            spawn client(other_owner, other_function, left, right, done);
        } else {
            spawn client(function_owner, function, left, right, done);
        }
    }
    for (index = 0; index < 8; index++) {
        <-done;
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, witness, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 42 || local.value != 0 || other_cell.value != 0 ||
        capture.value != original || other_capture.value != other_original || original == other_original) {
        raise "cross-owner ref became a local copy or migrated callback";
    }
    (status, value) = access->call(left, Proxyref->Close, 0, sys->millisec() + 10000);
    if (status != 0 || <-first_done != 1) {
        raise "cross-owner ref explicit close";
    }
    (status, value) = access->call(left, Proxyref->Read, 0, sys->millisec() + 10000);
    if (status != Proxyref->Disconnected) {
        raise "closed routed ref was revived";
    }
    left = nil;
    (status, value) = access->call(right, Proxyref->Read, 0, sys->millisec() + 10000);
    if (status != 0 || value != 42) {
        raise "independent routed ref revoked by alias close";
    }
    cycle := ref Cycle(nil, right);
    cycle.next = cycle;
    right = nil;
    cycle = nil;
    if (<-second_done != 1) {
        raise "last routed ref GC actor did not finish";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, witness, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 42) {
        raise "last ref GC revoked distinct source witness";
    }
    absent := Proxyowner->Handle(big 0, 0, big 0);
    (status, value) = proxy->call(function_owner, Proxyowner->Stop, absent, absent, absent, 0, 0, 10000);
    if (status != 0 || <-owner_done != 1) {
        raise "routed original function owner did not finish";
    }
    (status, value) = proxy->call(other_owner, Proxyowner->Stop, absent, absent, absent, 0, 0, 10000);
    if (status != 0 || <-other_done != 1) {
        raise "second routed function owner did not finish";
    }
    (status, value) = net->call(endpoint, Proxyowner->Drop, witness, empty, empty, 0, 0, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-network_done != 1) {
        raise "routed source network owner did not finish";
    }
    sys->fprint(sys->fildes(2), "PASS cross-owner proxy refs: original callback 8 clients atomic aliases Drop GC join\n");
}
