# Copyright (c) 2026 bhgv
# EN: A returned image in an abandoned buffered reply must release its remote owner.
# RU: Returned image в abandoned buffered reply должен освободить remote owner.
implement ProxyOrphanResult;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyOrphanResult: module { init: fn(); };
net: Proxynet; # Immutable initialized network adapter.


# EN/RU: Serve independent requests until terminal Stop; signal after broker retirement.
# Обслуживать independent requests до terminal Stop; сообщить broker retirement.
connect(input, output: ref Sys->FD, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, output, big 12345, big 67890, 5000, ready);
    stopped <-= 1;
}


# EN/RU: Enter a real fetch; peer withholds reply until this caller is retired.
# Войти в actual fetch; peer удерживает reply до retirement этого caller.
caller(endpoint: ref Proxynet->Endpoint, variable, function: Proxynet->Image, started: chan of int)
{
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    net->fetch(endpoint, function, variable, variable, 2, 71, Proxyhost->Variable, 5000);
    raise "result caller survived forced retirement";
}


# EN/RU: Kill fetch caller before reply, observe orphan GC release and keep originals usable.
# Уничтожить fetch caller до reply, наблюдать orphan GC release; originals остаются живы.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    stopped := chan[1] of int;
    spawn connect(sys->fildes(0), sys->fildes(1), stopped, ready);
    (status, endpoint, variable, function) := <-ready;
    if (status != 0) {
        raise "orphan result greeting";
    }
    started := chan[1] of int;
    spawn caller(endpoint, variable, function, started);
    child := <-started;
    sys->sleep(100);
    if (control->killwait(child) != Processctl->Retired) {
        raise "result caller failed to retire";
    }
    sys->fprint(sys->fildes(2), "PASS retired result caller before reply\n");
    released := 0;
    for (attempt := 0; attempt < 1000 && released < 1; attempt++) {
        (status, released) = net->collected(endpoint, 10000);
        sys->sleep(1);
    }
    if (released < 1) {
        raise "abandoned result reply leaked its image owner";
    }
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, 3, 10000);
    if (status != 0 || value != 16) {
        raise "orphan result revoked original live arguments";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "orphan result broker cleanup";
    }
    sys->fprint(sys->fildes(2), "PASS orphan result reply image GC released exactly once\n");
}
