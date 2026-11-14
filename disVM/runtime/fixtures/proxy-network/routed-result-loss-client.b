# Copyright (c) 2026 bhgv
# EN: A lost completed result is Unknown, never an adopted image or replay permission.
# RU: Потерянный completed result — Unknown, не adopted image и не разрешение replay.
implement ProxyRoutedLoss;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyRoutedLoss: module { init: fn(); };


# EN/RU: Actual B result completes before relay loss; queued owners must terminate safely.
# Actual B result завершается до relay loss; queued owners обязаны завершиться безопасно.
init()
{
    sys := load Sys Sys->PATH;
    net := load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 33333, big 44444, 60000, ready);
    (status, endpoint, local, function) := <-ready;
    (a, first) := net->importref(endpoint, function, 0, 10000);
    (b, second) := net->importref(endpoint, function, 0, 10000);
    if (status != 0 || a != 0 || b != 0) {
        raise "routed loss imports";
    }
    (unknown, image) := net->routedfetch(endpoint, function, first, second, 2, 1, 60000);
    (disconnected, unused) := net->call(endpoint, Proxyowner->Read, local, first, second, 0, 0, 10000);
    if (unknown != Proxyowner->Unknown || image.lease != nil || disconnected != Proxynet->Disconnected) {
        raise "lost completed routed result adopted ownership or stayed usable";
    }
    sys->fprint(sys->fildes(2), "PASS lost routed result: Unknown empty image disconnected no replay\n");
}
