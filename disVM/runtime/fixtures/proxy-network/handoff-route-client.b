# Copyright (c) 2026 bhgv
# EN: Local actor handoffs keep original absolute deadlines through delayed dispatch.
# RU: Local actor handoffs сохраняют original absolute deadlines при delayed dispatch.
implement HandoffRoute;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
HandoffRoute: module { init: fn(); };
net: Proxynet; # Immutable source broker interface.


# EN/RU: Report only natural broker retirement after all original owners are released.
# Сообщить лишь natural broker retirement после release всех original owners.
connect(fd: ref Sys->FD, ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image), done: chan of int)
{
    net->start(fd, fd, big 12345, big 67890, 60000, ready);
    done <-= 1;
}


# EN/RU: Start the independent variable actor with the supplied absolute startup deadline.
# Запустить independent variable actor с переданным absolute startup deadline.
attach(route: Proxyroute, endpoint: ref Proxynet->Endpoint, image: Proxynet->Image,
       deadline: int, ready: chan of (int, ref Proxyref->Ref), done: chan of int)
{
    route->startuntil(endpoint, image, 60000, deadline, ready);
    done <-= 1;
}


# EN: kind zero delays a Write handoff; one delays startup Retain. Return semantic
# success after actual source ownership and exercise roots have retired.
# RU: kind zero задерживает Write handoff, one задерживает startup Retain. Вернуть
# semantic success после retirement actual source ownership и exercise roots.
exercise(kind: int): int
{
    sys := load Sys Sys->PATH;
    route := load Proxyroute Proxyroute->PATH;
    access := load Proxyref Proxyref->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    route->init();
    access->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    stopped := chan[1] of int;
    spawn connect(sys->fildes(0), ready, stopped);
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0) {
        raise "handoff source greeting";
    }
    attached := chan[1] of (int, ref Proxyref->Ref);
    done := chan[1] of int;
    budget := 10000;
    if (kind == 1) {
        budget = 1000;
    }
    spawn attach(route, endpoint, variable, sys->millisec() + budget, attached, done);
    (status, source) := <-attached;
    correct := status == Proxyref->NotSent && source == nil;
    if (kind == 0) {
        if (status != 0 || source == nil) {
            raise "handoff source attachment";
        }
        (offered, unused) := access->call(source, Proxyref->Write, 999, sys->millisec() + 1000);
        if (offered != Proxyref->Unknown) {
            raise "handoff Write was not originally offered";
        }
        deadline := sys->millisec() + 10000;
        gate := sys->open("handoff-route-finished", Sys->OREAD);
        while (gate == nil && sys->millisec() < deadline) {
            sys->sleep(5);
            gate = sys->open("handoff-route-finished", Sys->OREAD);
        }
        if (gate == nil) {
            raise "handoff completion gate";
        }
        gate = nil;
        (read, value) := access->call(source, Proxyref->Read, 0, sys->millisec() + 10000);
        if (read != 0) {
            raise "handoff source became unusable";
        }
        correct = value == 10;
    } else if (status != 0 && status != Proxyref->NotSent) {
        raise "handoff startup failed outside deadline boundary";
    }
    if (source != nil) {
        (closed, unused2) := access->call(source, Proxyref->Close, 0, sys->millisec() + 10000);
        if (closed != 0) {
            raise "handoff independent source Close";
        }
    }
    source = nil;
    if (<-done != 1) {
        raise "handoff source actor retirement";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (first, unused3) := net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    (second, unused4) := net->call(endpoint, Proxyowner->Drop, function, empty, empty, 0, 0, 10000);
    (last, unused5) := net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (first != 0 || second != 0 || last != 0 || <-stopped != 1) {
        raise "handoff original source ownership";
    }
    return correct;
}


# EN/RU: Fail after exercise roots retire, so omissions require clean native lifetimes.
# Отказать после retirement exercise roots, чтобы omissions требовали clean native lifetimes.
init()
{
    if (!exercise(0)) {
        raise "source handoff renewed original deadline";
    }
    sys := load Sys Sys->PATH;
    sys->fprint(sys->fildes(2), "PASS source handoff: original deadline no late effect or startup clone\n");
}
