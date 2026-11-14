# Copyright (c) 2026 bhgv
# EN: Independent source actors close concurrently after their original function owner joins.
# RU: Independent source actors закрываются одновременно после join original function owner.
implement ParallelCleanup;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyregistry.m";
ParallelCleanup: module { init: fn(); };
net: Proxynet; # Immutable shared network interface.
owner: Proxyowner; # Immutable original function owner interface.


# EN/RU: Keep the actual source broker resident until Drop/Stop cleanup finishes.
# Удерживать actual source broker до завершения Drop/Stop cleanup.
connect(fd: ref Sys->FD, ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image), done: chan of int)
{
    net->start(fd, fd, big 12345, big 67890, 60000, ready);
    done <-= 1;
}


# EN/RU: Publish PID and own the original local function/Cell on this permanent Prog.
# Передать PID и владеть original local function/Cell на этом permanent Prog.
own(handler: Proxyhandler, cell, capture: ref Proxyhost->Cell, started: chan of int,
    ready: chan of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle))
{
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    owner->serve(handler, cell, capture, ready);
}


# EN/RU: Release original greeting owners only after independent clones have all retired.
# Освободить original greeting owners лишь после retirement independent clones.
finish(endpoint: ref Proxynet->Endpoint, variable, function: Proxynet->Image, done: chan of int)
{
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (first, unused) := net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    (second, unused2) := net->call(endpoint, Proxyowner->Drop, function, empty, empty, 0, 0, 10000);
    (stopped, unused3) := net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (first != 0 || second != 0 || stopped != 0 || <-done != 1) {
        raise "parallel cleanup original source ownership";
    }
}


# EN: Import two actual independent source views, join original function owner,
# close the registry, then prove all source/native ownership remains balanced.
# RU: Import два actual independent source views, join original function owner,
# закрыть registry, затем подтвердить balanced source/native ownership.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    registry := load Proxyregistry Proxyregistry->PATH;
    handler := load Proxyhandler "handler.dis";
    net = load Proxynet Proxynet->PATH;
    owner = load Proxyowner Proxyowner->PATH;
    net->init();
    owner->init();
    registry->init();
    first := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    second := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    done1 := chan[1] of int;
    done2 := chan[1] of int;
    spawn connect(sys->fildes(0), first, done1);
    spawn connect(sys->fildes(1), second, done2);
    (status1, endpoint1, variable1, function1) := <-first;
    (status2, endpoint2, variable2, function2) := <-second;
    if (status1 != 0 || status2 != 0) {
        raise "parallel cleanup source greetings";
    }
    cell := ref Proxyhost->Cell(42);
    capture := ref Proxyhost->Cell(0);
    started := chan[1] of int;
    ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    spawn own(handler, cell, capture, started, ready);
    pid := <-started;
    (endpoint, variable, function) := <-ready;
    sources := array[2] of ref Proxyregistry->Source;
    sources[0] = ref Proxyregistry->Source(endpoint1, variable1);
    sources[1] = ref Proxyregistry->Source(endpoint2, variable2);
    context := registry->create(sources, endpoint, 60000);
    absent := Proxyowner->Handle(big 0, 0, big 0);
    (handled1, imported1, unused1, view1, kind1) := registry->dispatch(context, Proxyowner->ImportRoute,
        function, absent, absent, 0, 0, sys->millisec() + 10000);
    (handled2, imported2, unused2, view2, kind2) := registry->dispatch(context, Proxyowner->ImportRoute,
        function, absent, absent, 0, 1, sys->millisec() + 10000);
    if (!handled1 || !handled2 || imported1 != 0 || imported2 != 0 || kind1 != 1 || kind2 != 1) {
        raise "parallel cleanup independent imports";
    }
    (stopped, unused3) := owner->call(endpoint, Proxyowner->Stop, variable, absent, absent, 0, 0, 10000);
    control->killwait(pid);
    if (stopped != 0) {
        raise "parallel cleanup original function owner";
    }
    registry->close(context);
    registry->close(context);
    if (!context.closed || context.views[0].source != nil || context.views[1].source != nil ||
        context.actors[0] != nil || context.actors[1] != nil || cell.value != 42) {
        raise "parallel cleanup local roots or actor completion";
    }
    finish(endpoint1, variable1, function1, done1);
    finish(endpoint2, variable2, function2, done2);
    sys->fprint(sys->fildes(2), "PASS parallel cleanup: independent actors joined original owners balanced local=42\n");
}
