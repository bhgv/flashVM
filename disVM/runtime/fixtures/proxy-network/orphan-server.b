# Copyright (c) 2026 bhgv
# EN: Public connection caller retirement preserves finite private session cleanup.
# RU: Retirement public connection caller сохраняет конечный private session cleanup.
implement ProxyOrphanServer;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyregistry.m";
include "proxyserver.m";
ProxyOrphanServer: module { init: fn(); };
server: Proxyserver; # Immutable initialized connection supervisor module.


# EN/RU: Publish public caller PID, then serve until forced caller retirement.
# Передать PID public caller и обслуживать до его forced retirement.
connection(handler: Proxyhandler, cell, capture: ref Proxyhost->Cell, started: chan of int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    server->serve(sys->fildes(0), sys->fildes(1), big 12345, big 67890, 3000, handler, cell, capture);
    raise "public connection caller survived forced retirement";
}


# EN/RU: Retire caller only after the original callback owner published its identity.
# Завершить caller лишь после публикации identity исходного callback owner.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    server = load Proxyserver Proxyserver->PATH;
    handler := load Proxyhandler "handler.dis";
    server->init();
    cell := ref Proxyhost->Cell(10);
    capture := ref Proxyhost->Cell(0);
    started := chan[1] of int;
    spawn connection(handler, cell, capture, started);
    caller := <-started;
    for (attempt := 0; capture.value == 0 && attempt < 10000; attempt++) {
        sys->sleep(1);
    }
    sys->sleep(50);
    if (capture.value == 0 || control->killwait(caller) != Processctl->Retired) {
        raise "public connection caller did not retire after owner startup";
    }
    sys->fprint(sys->fildes(2), "PASS retired connection caller leaves finite rooted session\n");
}
