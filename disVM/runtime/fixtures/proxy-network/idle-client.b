# Copyright (c) 2026 bhgv
# EN: Unused client images cannot keep an idle broker resident indefinitely.
# RU: Неиспользуемые client images не удерживают idle broker бесконечно.
implement ProxyIdleClient;
include "sys.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyIdleClient: module { init: fn(); };
net: Proxynet; # Immutable initialized network module.


# EN/RU: Publish finite-idle broker completion independently of its returned images.
# Опубликовать finite-idle broker completion независимо от возвращённых images.
connect(input, output: ref Sys->FD, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 1000, ready);
    stopped <-= 1;
}


# EN/RU: Retain images past idle expiry and require immediate terminal refusal.
# Сохранить images после idle expiry и требовать немедленный terminal refusal.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    stopped := chan[1] of int;
    spawn connect(sys->fildes(0), sys->fildes(1), stopped, ready);
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0 || <-stopped != 1) {
        raise "idle network broker did not retire";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0));
    (status, unused) := net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != Proxynet->Disconnected) {
        raise "idle image attempted a fresh wire call";
    }
    sys->fprint(sys->fildes(2), "PASS idle network images are terminal after broker retirement\n");
}
