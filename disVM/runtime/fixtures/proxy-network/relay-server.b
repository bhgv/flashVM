# Copyright (c) 2026 bhgv
# EN: VM B forwards authorized views of VM A to an independent caller VM C.
# RU: VM B передаёт authorized views VM A независимому caller VM C.
implement ProxyRelayServer;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyregistry.m";
include "proxyserver.m";
ProxyRelayServer: module { init: fn(); };
net: Proxynet; # Immutable source connection client.


# EN/RU: Source A uses full-duplex stdin; retain it through all view/actor cleanup.
# Source A использует full-duplex stdin; удержать до view/actor cleanup.
connect(input: ref Sys->FD, done: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    net->start(input, input, big 12345, big 67890, 15000, ready);
    done <-= 1;
}


# EN/RU: Function service uses full-duplex stdout; stderr remains diagnostics only.
# Function service использует full-duplex stdout; stderr остаётся diagnostics.
init()
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    server := load Proxyserver Proxyserver->PATH;
    handler := load Proxyhandler "handler.dis";
    net->init();
    server->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    done := chan[1] of int;
    spawn connect(sys->fildes(0), done, ready);
    (status, endpoint, variable, function) := <-ready;
    if (status != 0) {
        raise "relay source greeting";
    }
    sources := array[1] of ref Proxyregistry->Source;
    sources[0] = ref Proxyregistry->Source(endpoint, variable);
    local := ref Proxyhost->Cell(0);
    capture := ref Proxyhost->Cell(0);
    result := server->registered(sys->fildes(1), sys->fildes(1), big 33333, big 44444,
        12000, handler, local, capture, sources);
    lost := endpoint.state.closed;
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    value: int;
    (status, value) = net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Drop, function, empty, empty, 0, 0, 10000);
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if ((!lost && status != 0) || <-done != 1 || local.value != 0) {
        raise "relay cleanup changed local storage or lost source owner";
    }
    sys->fprint(sys->fildes(2), "PASS relay function server: result=%d local=%d source actors joined\n", result, local.value);
}
