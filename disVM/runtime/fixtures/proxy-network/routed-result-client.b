# Copyright (c) 2026 bhgv
# EN: Routed callback results retain live foreign variables independently of arguments.
# RU: Results routed callback удерживают live foreign variables независимо от arguments.
implement ProxyRoutedResult;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyRoutedResult: module { init: fn(); };
net: Proxynet; # Immutable typed network interface.


# EN/RU: Last result image in an unreachable cycle requires concurrent cycle collection.
# Last result image в unreachable cycle требует concurrent cycle collection.
Cycle: adt
{
    next: ref Cycle; # Unreachable local self-cycle.
    image: Proxynet->Image; # Sole remaining counted result owner.
};


# EN/RU: Each concurrent caller owns a distinct result token; ordinary copies share it.
# Каждый concurrent caller владеет distinct result token; ordinary copies его разделяют.
client(endpoint: ref Proxynet->Endpoint, function, first, second: Proxynet->Image, done: chan of int)
{
    (status, image) := net->routedfetch(endpoint, function, first, second, 2, 1, 60000);
    if (status != 0 || image.lease == nil || image.lease == second.lease) {
        raise "concurrent routed result ownership";
    }
    (read, value) := net->call(endpoint, Proxyowner->Read, image, image, image, 0, 0, 60000);
    if (read != 0 || value < 22) {
        raise "concurrent routed result lost original storage";
    }
    done <-= 1;
}


# EN: Exercise live result identity, native Retain, forbidden wrappers, partial
# effects, eight callers and last-image GC through three actual VMs.
# RU: Проверить live result identity, native Retain, forbidden wrappers, partial
# effects, восемь callers и last-image GC через три actual VMs.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 33333, big 44444, 60000, ready);
    (status, endpoint, local, function) := <-ready;
    if (status != 0) {
        raise "routed result greeting";
    }
    (a, first) := net->importref(endpoint, function, 0, 10000);
    (b, second) := net->importref(endpoint, function, 0, 10000);
    if (a != 0 || b != 0) {
        raise "routed result imports";
    }
    (invalid, refused) := net->routedfetch(endpoint, local, first, second, 2, 1000, 10000);
    if (invalid != 3 || refused.lease != nil) {
        raise "routed result accepted variable as original function";
    }
    (invalid, refused) = net->routedfetch(endpoint, function, first, second, 2, -2, 10000);
    if (invalid != 3 || refused.lease != nil) {
        raise "nil routed result became an image";
    }
    (invalid, refused) = net->routedfetch(endpoint, function, first, second, 2, -3, 10000);
    if (invalid != 3 || refused.lease != nil) {
        raise "unregistered routed result wrapper escaped";
    }
    (created, result) := net->routedfetch(endpoint, function, first, second, 2, 1, 10000);
    if (created != 0 || result.lease == nil || result.lease == second.lease ||
        result.owner.slot != second.owner.slot || result.owner.generation != second.owner.generation) {
        raise "routed result binding lost live alias";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (dropped, value) := net->call(endpoint, Proxyowner->Drop, second, empty, empty, 0, 0, 10000);
    (cloned, retained) := net->clone(endpoint, result, 10000);
    if (dropped != 0 || cloned != 0) {
        raise "routed result native ownership lost after original Drop";
    }
    (written, unused) := net->call(endpoint, Proxyowner->Write, result, empty, empty, 0, 20, 10000);
    (read, observed) := net->call(endpoint, Proxyowner->Read, first, empty, empty, 0, 0, 10000);
    if (written != 0 || read != 0 || observed != 20) {
        raise "routed result became a snapshot or wrong source";
    }
    done := chan of int;
    for (index := 0; index < 8; index++) {
        spawn client(endpoint, function, first, result, done);
    }
    for (index = 0; index < 8; index++) {
        <-done;
    }
    count := 0;
    for (attempt := 0; attempt < 1000 && count < 8; attempt++) {
        (status, count) = net->collected(endpoint, 10000);
        sys->sleep(1);
    }
    failed: int;
    (failed, refused) = net->routedfetch(endpoint, function, first, result, 2, -1, 10000);
    (read, observed) = net->call(endpoint, Proxyowner->Read, retained, empty, empty, 0, 0, 10000);
    if (count != 8 || failed != Proxyowner->OwnerError || refused.lease != nil || read != 0 || observed != 37) {
        raise "routed result GC or partial effect contract";
    }
    (function_drop, ignored_function) := net->call(endpoint, Proxyowner->Drop, function, empty, empty, 0, 0, 10000);
    stale_function: int;
    (stale_function, refused) = net->routedfetch(endpoint, function, first, result, 2, 999, 10000);
    if (function_drop != 0 || stale_function != Proxynet->Stale || refused.lease != nil) {
        raise "routed result function image did not retire independently";
    }
    net->call(endpoint, Proxyowner->Drop, first, empty, empty, 0, 0, 10000);
    net->call(endpoint, Proxyowner->Drop, result, empty, empty, 0, 0, 10000);
    (written, unused) = net->call(endpoint, Proxyowner->Write, retained, empty, empty, 0, 42, 10000);
    if (written != 0) {
        raise "returned ref did not survive all argument drops";
    }
    cycle := ref Cycle(nil, retained);
    cycle.next = cycle;
    retained = empty;
    cycle = nil;
    for (attempt = 0; attempt < 1000 && count < 9; attempt++) {
        (status, count) = net->collected(endpoint, 10000);
        sys->sleep(1);
    }
    (read, observed) = net->call(endpoint, Proxyowner->Read, local, empty, empty, 0, 0, 10000);
    (stopped, ignored) := net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (count != 9 || read != 0 || observed != 0 || stopped != 0) {
        raise "routed result terminal roots or local storage";
    }
    sys->fprint(sys->fildes(2), "PASS routed live results: native retain aliases 8 callers partial effects cyclic GC\n");
}
