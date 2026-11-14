# Copyright (c) 2026 bhgv
# EN: A live variable actor completes accepted work after its access caller retires.
# RU: Live variable actor завершает accepted work после retirement access caller.
implement RefOrphan;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
RefOrphan: module { init: fn(); };
access: Proxyref; # Immutable variable-only access interface.


# EN/RU: Signal only after independent source actor releases its own remote clone.
# Сообщить лишь после release independent source actor своего remote clone.
attach(route: Proxyroute, endpoint: ref Proxynet->Endpoint, image: Proxynet->Image,
       ready: chan of (int, ref Proxyref->Ref), done: chan of int)
{
    route->start(endpoint, image, 30000, ready);
    done <-= 1;
}


# EN/RU: Offer actual foreign Add with a sixty-second budget before parent retirement.
# Предложить actual foreign Add с sixty-second budget до parent retirement.
caller(source: ref Proxyref->Ref, started: chan of int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    access->call(source, Proxyref->Add, 1, sys->millisec() + 60000);
    raise "ref orphan caller resumed before retirement";
}


# EN/RU: Observe completed A effect, retire caller, then retain actual variable/actor lifetimes.
# Увидеть completed A effect, завершить caller, сохранить actual variable/actor lifetimes.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    net := load Proxynet Proxynet->PATH;
    route := load Proxyroute Proxyroute->PATH;
    access = load Proxyref Proxyref->PATH;
    net->init();
    route->init();
    access->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 12345, big 67890, 60000, ready);
    (status, endpoint, variable, function) := <-ready;
    if (status != 0) {
        raise "ref orphan greeting";
    }
    ref_ready := chan[1] of (int, ref Proxyref->Ref);
    done := chan[1] of int;
    spawn attach(route, endpoint, variable, ref_ready, done);
    (attached, source) := <-ref_ready;
    if (attached != 0) {
        raise "ref orphan attachment";
    }
    started := chan[1] of int;
    spawn caller(source, started);
    child := <-started;
    deadline := sys->millisec() + 15000;
    gate := sys->open("ref-orphan-held", Sys->OREAD);
    while (gate == nil && sys->millisec() < deadline) {
        sys->sleep(5);
        gate = sys->open("ref-orphan-held", Sys->OREAD);
    }
    if (gate == nil || control->killwait(child) != Processctl->Retired) {
        raise "ref orphan effect gate or caller retirement";
    }
    gate = nil;
    sys->fprint(sys->fildes(2), "PASS ref orphan caller retired before reply\n");
    (read, value) := access->call(source, Proxyref->Read, 0, sys->millisec() + 10000);
    (closed, unused) := access->call(source, Proxyref->Close, 0, sys->millisec() + 10000);
    if (read != 0 || value != 11 || closed != 0 || <-done != 1) {
        raise "ref orphan effect replay or actor ownership";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    net->call(endpoint, Proxyowner->Drop, function, empty, empty, 0, 0, 10000);
    (stopped, ignored) := net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (stopped != 0) {
        raise "ref orphan source Stop";
    }
    sys->fprint(sys->fildes(2), "PASS ref orphan: actual effect once original variable actor alarm joined\n");
}
