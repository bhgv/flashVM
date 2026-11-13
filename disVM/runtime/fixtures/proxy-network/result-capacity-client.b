# Copyright (c) 2026 bhgv
# EN: Injected result reservation refusal must precede any callback effect.
# RU: Внедрённый result reservation refusal обязан предшествовать callback effect.
implement ProxyResultCapacityClient;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyResultCapacityClient: module { init: fn(); };
net: Proxynet; # Immutable initialized network library.


# EN/RU: Run original broker, then acknowledge its natural local Stop completion.
# Исполнить original broker, затем подтвердить natural local Stop.
connect(input, output: ref Sys->FD, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 1000, ready);
    stopped <-= 1;
}


# EN/RU: Refuse result locally, verify original cell and join the unchanged session.
# Отказать result локально, проверить original cell и завершить session.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    stopped := chan[1] of int;
    spawn connect(sys->fildes(0), sys->fildes(1), stopped, ready);
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0) {
        raise "network fixture greeting refused";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (status, image) := net->fetch(endpoint, function, variable, variable, 2, -1, Proxyhost->Variable, 10000);
    if (status != Proxyhost->Capacity || image.lease != nil) {
        raise "result reservation capacity did not precede callback";
    }
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 10) {
        raise "result reservation refusal executed callback effect";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "result capacity broker terminal cleanup";
    }
    sys->fprint(sys->fildes(2), "PASS result capacity refused before actual callback effect\n");
}
