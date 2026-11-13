# Copyright (c) 2026 bhgv
# EN: Real DisVM server owns a mutable cell and user callback across a connected byte stream.
# RU: Настоящий DisVM server владеет mutable cell и user callback через connected byte stream.
implement ProxyNetworkServer;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyregistry.m";
include "proxyserver.m";
ProxyNetworkServer: module { init: fn(); };


# EN/RU: Borrow inherited trusted duplex stdin/stdout; report final original value on stderr.
# Заимствовать inherited trusted duplex stdin/stdout; вывести original value в stderr.
init()
{
    sys := load Sys Sys->PATH;
    server := load Proxyserver Proxyserver->PATH;
    handler := load Proxyhandler "handler.dis";
    if (server == nil || handler == nil) {
        raise "network owner fixture modules";
    }
    server->init();
    cell := ref Proxyhost->Cell(10);
    capture := ref Proxyhost->Cell(0);
    result := server->serve(sys->fildes(0), sys->fildes(1), big 12345, big 67890, 5000, handler, cell, capture);
    sys->fprint(sys->fildes(2), "PASS network server joined owner: result=%d value=%d\n", result, cell.value);
}
