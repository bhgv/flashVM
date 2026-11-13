# Copyright (c) 2026 bhgv
# EN: Queued expired offers cannot send effects, consume leases or close the live broker.
# RU: Queued expired offers не отправляют effects, не потребляют leases и не закрывают broker.
implement ProxyQueuedDeadline;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyQueuedDeadline: module { init: fn(); };
net: Proxynet; # Immutable broker interface after init.


# EN/RU: Hold the first wire call while a short-lived second caller retires.
# Удерживать первый wire call до retirement второго caller с коротким deadline.
read(endpoint: ref Proxynet->Endpoint, image: Proxynet->Image, done: chan of (int, int))
{
    (status, value) := net->call(endpoint, Proxyowner->Read, image, image, image, 0, 0, 10000);
    done <-= (status, value);
}


# EN: Actual broker keeps queued requests rooted; the peer delays its first reply
# until this caller reports Unknown. Expired local lifecycle offers must be inert.
# RU: Actual broker удерживает queued requests; peer задерживает первый reply до
# сообщения Unknown. Просроченные local lifecycle offers обязаны быть inert.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 12345, big 67890, 15000, ready);
    (status, endpoint, variable, function) := <-ready;
    if (status != 0) {
        raise "queued deadline greeting";
    }
    done := chan[1] of (int, int);
    spawn read(endpoint, variable, done);
    # Wait for independent peer confirmation that the first wire Read is in flight.
    gate := sys->open("queued-ready", Sys->OREAD);
    attempts := 0;
    while (gate == nil && attempts++ < 2000) {
        sys->sleep(5);
        gate = sys->open("queued-ready", Sys->OREAD);
    }
    if (gate == nil) {
        raise "first queued deadline request never reached peer";
    }
    gate = nil;
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Write, variable, variable, variable, 0, 999, 1000);
    if (status != Proxyowner->Unknown) {
        raise sys->sprint("queued write was not offered: status=%d", status);
    }
    sys->fprint(sys->fildes(2), "QUEUED expired caller retired\n");
    (status, value) = <-done;
    if (status != 0 || value != 10) {
        raise "first queued deadline read failed";
    }
    operations := array[] of { Proxyowner->Drop, Proxyowner->Retain, Proxyowner->ReturnVariable,
        Proxyowner->ImportRoute, Proxyowner->Stop };
    for (index := 0; index < len operations; index++) {
        reply := chan[1] of Proxynet->Reply;
        target := variable;
        if (operations[index] == Proxyowner->ReturnVariable || operations[index] == Proxyowner->ImportRoute) {
            target = function;
        }
        endpoint.requests <-= ref Proxynet->Request(operations[index], target, variable, variable,
            0, 999, sys->millisec() - 1, reply);
        result := <-reply;
        if (result.status != Proxyowner->NotSent || result.image.lease != nil || endpoint.state.closed) {
            raise "expired queued lifecycle mutated ownership or closed connection";
        }
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, variable, variable, 0, 0, 10000);
    if (status != 0 || value != 10) {
        raise "expired queued write executed or Drop consumed original lease";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, variable, variable, variable, 0, 0, 10000);
    if (status != 0) {
        raise "live Stop after expired queued Stop failed";
    }
    sys->fprint(sys->fildes(2), "PASS queued deadline: no late effect lifecycle inert broker usable\n");
}
