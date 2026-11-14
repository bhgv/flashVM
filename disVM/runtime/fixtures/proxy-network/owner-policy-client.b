# Copyright (c) 2026 bhgv
# EN: A private supervisor must not grant nested RPC authority to an active callback.
# RU: Private supervisor не предоставляет nested RPC authority active callback.
implement OwnerPolicy;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
OwnerPolicy: module { init: fn(); };


# EN/RU: Serve original capture with a fixture that tests nested canwait refusal.
# Обслуживать original capture с fixture, проверяющим nested canwait refusal.
own(proxy: Proxyowner, cell, capture: ref Proxyhost->Cell, done: chan of int,
    ready: chan of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle))
{
    handler := load Proxyhandler "owner-policy-handler.dis";
    proxy->serve(handler, cell, capture, ready);
    done <-= 1;
}


# EN/RU: Original owner remains live and unchanged after refusing a nested private offer.
# Original owner остаётся live и unchanged после отказа nested private offer.
init()
{
    sys := load Sys Sys->PATH;
    proxy := load Proxyowner Proxyowner->PATH;
    proxy->init();
    cell := ref Proxyhost->Cell(10);
    capture := ref Proxyhost->Cell(0);
    ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    done := chan[1] of int;
    spawn own(proxy, cell, capture, done, ready);
    (endpoint, variable, function) := <-ready;
    (status, value) := proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -776, 10000);
    (stopped, unused) := proxy->call(endpoint, Proxyowner->Stop, variable, variable, variable, 0, 0, 10000);
    if (stopped != 0 || <-done != 1 || cell.value != 10) {
        raise "owner policy cleanup or local mutation";
    }
    if (status != 0 || value != 10) {
        raise "nested owner call bypassed pre-spawn policy";
    }
    sys->fprint(sys->fildes(2), "PASS owner policy: nested RPC refused on original callback owner\n");
}
