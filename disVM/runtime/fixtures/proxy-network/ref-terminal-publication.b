# Copyright (c) 2026 bhgv
# EN: Successful Ref Close already rejects an immediate next access before actor retirement.
# RU: Успешный Ref Close уже запрещает следующий access до retirement actor.
implement RefTerminalPublication;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
RefTerminalPublication: module { init: fn(); };


# EN/RU: Own one source actor and signal only after its independent clone cleanup.
# Владеть source actor и сообщать лишь после cleanup independent clone.
attach(route: Proxyroute, endpoint: ref Proxynet->Endpoint, image: Proxynet->Image,
       ready: chan of (int, ref Proxyref->Ref), done: chan of int)
{
    route->start(endpoint, image, 60000, ready);
    done <-= 1;
}


# EN/RU: Borrow actual source FD; return immediate refusal after joining all source ownership.
# Borrow actual source FD; вернуть immediate refusal после join всего source ownership.
exercise(): int
{
    sys := load Sys Sys->PATH;
    net := load Proxynet Proxynet->PATH;
    route := load Proxyroute Proxyroute->PATH;
    access := load Proxyref Proxyref->PATH;
    net->init();
    route->init();
    access->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 12345, big 67890, 60000, ready);
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0) {
        raise "ref terminal publication greeting";
    }
    attached := chan[1] of (int, ref Proxyref->Ref);
    done := chan[1] of int;
    spawn attach(route, endpoint, variable, attached, done);
    (status, source) := <-attached;
    if (status != 0) {
        raise "ref terminal publication attach";
    }
    (closed, unused) := access->call(source, Proxyref->Close, 0, sys->millisec() + 10000);
    if (closed != 0) {
        raise "ref terminal publication lost successful Close";
    }
    (next, unused2) := access->call(source, Proxyref->Read, 0, sys->millisec() + 10000);
    refused := next == Proxyref->Disconnected;
    if (<-done != 1 || source.state.closed != Proxyref->Closed) {
        raise "ref terminal publication actor did not settle";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (first, unused3) := net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    (second, unused4) := net->call(endpoint, Proxyowner->Drop, function, empty, empty, 0, 0, 10000);
    (stopped, unused5) := net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (first != 0 || second != 0 || stopped != 0) {
        raise "ref terminal publication source cleanup";
    }
    return refused;
}


# EN/RU: Exercise frame releases Ref/Image roots before the exact omission diagnostic.
# Exercise frame освобождает Ref/Image roots до exact omission diagnostic.
init()
{
    if (!exercise()) {
        raise "ref terminal publication exposed reply before refusing reentry";
    }
    sys := load Sys Sys->PATH;
    sys->fprint(sys->fildes(2), "PASS ref terminal publication: successful Close fences immediate Read\n");
}
