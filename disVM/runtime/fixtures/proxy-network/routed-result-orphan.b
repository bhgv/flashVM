# Copyright (c) 2026 bhgv
# EN: A completed routed result outlives its caller and is released by actual image GC.
# RU: Completed routed result переживает caller и освобождается actual image GC.
implement ProxyRoutedOrphan;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyRoutedOrphan: module { init: fn(); };
net: Proxynet; # Immutable client interface.


# EN/RU: Parent retires this caller only after actual B callback completion is witnessed.
# Parent завершает caller лишь после подтверждения actual B callback completion.
caller(endpoint: ref Proxynet->Endpoint, function, first, second: Proxynet->Image, started: chan of int)
{
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    net->routedfetch(endpoint, function, first, second, 2, 1, 60000);
    raise "orphan routed result caller resumed before retirement";
}


# EN/RU: Keep original views, retire only the result caller, then require its GC Drop.
# Сохранить original views, завершить лишь result caller, требовать его GC Drop.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 33333, big 44444, 60000, ready);
    (status, endpoint, local, function) := <-ready;
    (a, first) := net->importref(endpoint, function, 0, 10000);
    (b, second) := net->importref(endpoint, function, 0, 10000);
    if (status != 0 || a != 0 || b != 0) {
        raise "orphan routed result imports";
    }
    started := chan[1] of int;
    spawn caller(endpoint, function, first, second, started);
    child := <-started;
    deadline := sys->millisec() + 15000;
    gate := sys->open("routed-result-held", Sys->OREAD);
    while (gate == nil && sys->millisec() < deadline) {
        sys->sleep(5);
        gate = sys->open("routed-result-held", Sys->OREAD);
    }
    if (gate == nil || control->killwait(child) != Processctl->Retired) {
        raise "routed result completion gate or caller retirement";
    }
    gate = nil;
    sys->fprint(sys->fildes(2), "PASS routed result caller retired before reply\n");
    collected := 0;
    for (wait := 0; wait < 1000 && collected < 1; wait++) {
        (status, collected) = net->collected(endpoint, 10000);
        sys->sleep(1);
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (read, value) := net->call(endpoint, Proxyowner->Read, second, empty, empty, 0, 0, 10000);
    if (collected < 1 || read != 0 || value != 12) {
        raise "orphan routed result lost original ownership or effect";
    }
    (retained, witness) := net->clone(endpoint, second, 10000);
    if (retained != 0) {
        raise "orphan routed result consumed original native ownership";
    }
    net->call(endpoint, Proxyowner->Drop, witness, empty, empty, 0, 0, 10000);
    net->call(endpoint, Proxyowner->Drop, first, empty, empty, 0, 0, 10000);
    net->call(endpoint, Proxyowner->Drop, second, empty, empty, 0, 0, 10000);
    (stopped, ignored) := net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (stopped != 0) {
        raise "orphan routed result stop";
    }
    sys->fprint(sys->fildes(2), "PASS orphan routed result: completed effect late image GC originals live\n");
}
