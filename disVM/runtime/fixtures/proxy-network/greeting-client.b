# Copyright (c) 2026 bhgv
# EN: Failed greeting returns an explicit status without publishing endpoint/images.
# RU: Failed greeting возвращает явный status без публикации endpoint/images.
implement ProxyGreetingClient;
include "sys.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyGreetingClient: module { init: fn(); };


# EN/RU: Refuse malformed authority/owner metadata before exposing a network capability.
# Отказать malformed authority/owner metadata до выдачи network capability.
init()
{
    sys := load Sys Sys->PATH;
    net := load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 12345, big 67890, 1000, ready);
    (status, endpoint, variable, function) := <-ready;
    if (status != Proxynet->Disconnected || endpoint != nil || variable.owner.session != big 0 ||
        function.owner.session != big 0) {
        raise "bad greeting exposed a capability";
    }
    sys->fprint(sys->fildes(2), "PASS bad greeting returns no endpoint or images\n");
}
