# Copyright (c) 2026 bhgv
# EN: Last managed image references release remote owners before session Stop.
# RU: Последние managed image references снимают remote owners до session Stop.
implement ProxyGcClient;
include "sys.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyGcClient: module { init: fn(); };
# EN/RU: An unreachable client cycle must not keep its image owner alive.
# Недостижимый client cycle не должен сохранять image owner.
Cycle: adt
{
    next: ref Cycle; # Self-cycle, collected by concurrent Cycles.
    image: Proxynet->Image; # Nested managed native lease owner.
};

net: Proxynet; # Immutable initialized owner-image library.


# EN/RU: Keep broker completion observable without retaining its returned images.
# Наблюдать broker completion без удержания возвращённых images.
connect(input, output: ref Sys->FD, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 5000, ready);
    stopped <-= 1;
}


# EN/RU: Wait for acknowledged GC-origin Releases, never a forced whole-world collection.
# Ждать acknowledged GC-origin Releases без forced whole-world collection.
collected(endpoint: ref Proxynet->Endpoint, wanted: int)
{
    sys := load Sys Sys->PATH;
    for (attempt := 0; attempt < 1000; attempt++) {
        (status, count) := net->collected(endpoint, 10000);
        if (status == 0 && count >= wanted) {
            return;
        }
        sys->sleep(1);
    }
    raise "remote image GC did not acknowledge expected releases";
}


# EN/RU: Release originals by GC, use retained clone, then collect that last image too.
# Снять originals через GC, использовать retained clone и собрать последний image.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    stopped := chan[1] of int;
    spawn connect(sys->fildes(0), sys->fildes(1), stopped, ready);
    (status, endpoint, variable, function) := <-ready;
    if (status != 0) {
        raise "GC client greeting";
    }
    (clone_status, retained) := net->clone(endpoint, variable, 10000);
    if (clone_status != 0) {
        raise "GC client Retain";
    }
    (function_status, retained_function) := net->clone(endpoint, function, 10000);
    if (function_status != 0) {
        raise "GC client function Retain";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    cycle := ref Cycle(nil, variable);
    cycle.next = cycle;
    variable = empty;
    function = empty;
    cycle = nil;
    collected(endpoint, 2);
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Read, retained, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 10) {
        raise "GC of original images revoked distinct retained owner";
    }
    (status, value) = net->call(endpoint, Proxyowner->Invoke, retained_function,
        retained, retained, 2, 2, 10000);
    if (status != 0 || value != 14) {
        raise "GC of original function revoked retained callable or ref arguments";
    }
    retained_function = empty;
    collected(endpoint, 3);
    (status, value) = net->call(endpoint, Proxyowner->Read, retained, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 14) {
        raise "function image collection revoked its borrowed ref argument";
    }
    retained = empty;
    collected(endpoint, 4);
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "GC client terminal cleanup";
    }
    sys->fprint(sys->fildes(2), "PASS network image GC: original variable/function and last clone released\n");
}
