# Copyright (c) 2026 bhgv
# EN: Reject corrupted replies without replay and retire a terminal local broker.
# RU: Отвергать corrupted replies без replay и завершать terminal local broker.
implement ProxyNetworkFailureClient;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyNetworkFailureClient: module { init: fn(); };
net: Proxynet; # Immutable initialized network library.


# EN/RU: Run original broker, then acknowledge its natural local Stop completion.
# Исполнить original broker, затем подтвердить natural local Stop.
connect(input, output: ref Sys->FD, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 1000, ready);
    stopped <-= 1;
}


# EN/RU: A bad reply becomes Unknown; subsequent calls refuse without another wire effect.
# Плохой reply означает Unknown; следующие calls отказывают без нового wire effect.
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
    (status, unused) := net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != Proxyowner->Unknown) {
        raise "corrupt or missing network reply was trusted";
    }
    (status, unused) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != Proxynet->Disconnected) {
        raise "closed connection attempted another call";
    }
    (status, unused) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != Proxynet->Disconnected || <-stopped != 1) {
        raise "terminal network broker did not retire";
    }
    sys->fprint(sys->fildes(2), "PASS network unknown fenced without replay\n");
}
