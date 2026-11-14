# Copyright (c) 2026 bhgv
# EN: Original owner must reject expired queued work before pins and lifecycle effects.
# RU: Original owner обязан отклонять expired queued work до pins и lifecycle effects.
implement ProxyOwnerDeadline;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
ProxyOwnerDeadline: module { init: fn(); };
proxy: Proxyowner; # Immutable owner interface.


# EN/RU: Serve actual Cell on its original dedicated Prog; signal clean owner return.
# Обслуживать actual Cell на original dedicated Prog; сообщить clean owner return.
own(cell, capture: ref Proxyhost->Cell, done: chan of int,
    ready: chan of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle))
{
    handler := load Proxyhandler "queued-handler.dis";
    proxy->serve(handler, cell, capture, ready);
    done <-= 1;
}


# EN/RU: Keep the owner inside a real callback until the short queued caller retires.
# Удержать owner внутри real callback до retirement short queued caller.
hold(endpoint: ref Proxyowner->Endpoint, function, variable: Proxyowner->Handle, done: chan of (int, int))
{
    (status, value) := proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -777, 10000);
    done <-= (status, value);
}


# EN/RU: Exercise expired ordinary/export/lifecycle requests against actual rooted storage.
# Проверить expired ordinary/export/lifecycle requests на actual rooted storage.
init()
{
    sys := load Sys Sys->PATH;
    proxy = load Proxyowner Proxyowner->PATH;
    proxy->init();
    cell := ref Proxyhost->Cell(10);
    capture := ref Proxyhost->Cell(0);
    ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    retired := chan[1] of int;
    spawn own(cell, capture, retired, ready);
    (endpoint, variable, function) := <-ready;
    done := chan[1] of (int, int);
    spawn hold(endpoint, function, variable, done);
    gate := sys->open("owner-entered", Sys->OREAD);
    attempts := 0;
    while (gate == nil && attempts++ < 2000) {
        sys->sleep(5);
        gate = sys->open("owner-entered", Sys->OREAD);
    }
    if (gate == nil) {
        raise "owner deadline callback never entered";
    }
    gate = nil;
    (status, value) := proxy->call(endpoint, Proxyowner->Write, variable, variable, variable, 0, 999, 1000);
    if (status != Proxyowner->Unknown) {
        raise "short original-owner request never queued";
    }
    release := sys->create("owner-release", Sys->OWRITE, 8r666);
    if (release == nil) {
        raise "owner deadline callback release";
    }
    release = nil;
    (status, value) = <-done;
    if (status != 0 || value != 10) {
        raise "owner deadline initial callback result";
    }
    (status, value) = proxy->call(endpoint, Proxyowner->Read, variable, variable, variable, 0, 0, 10000);
    if (status != 0 || value != 10) {
        proxy->call(endpoint, Proxyowner->Stop, variable, variable, variable, 0, 0, 10000);
        <-retired;
        raise sys->sprint("expired original-owner Write executed: value=%d", value);
    }
    operations := array[] of { Proxyowner->Read, Proxyowner->Write, Proxyowner->Add,
        Proxyowner->Invoke, Proxyowner->Retain, Proxyowner->Drop, Proxyowner->Stop,
        Proxyowner->ReturnVariable, Proxyowner->ReturnFunction, Proxyowner->Expose };
    absent := Proxyowner->Handle(big 0, 0, big 0);
    for (index := 0; index < len operations; index++) {
        reply := chan[1] of (int, int, Proxyowner->Handle);
        endpoint.requests <-= ref Proxyowner->Request(operations[index], function, variable, variable,
            2, 999, 0, nil, nil, sys->millisec() - 1, cell, nil, reply);
        (expired, unused, returned) := <-reply;
        if (expired != Proxyowner->NotSent || returned.session != big 0) {
            raise "expired original-owner lifecycle or export admitted";
        }
    }
    (status, value) = proxy->call(endpoint, Proxyowner->Read, variable, absent, absent, 0, 0, 10000);
    if (status != 0 || value != 10 || cell.value != 10) {
        raise "expired original-owner work changed storage or refs";
    }
    (status, value) = proxy->call(endpoint, Proxyowner->Stop, absent, absent, absent, 0, 0, 10000);
    if (status != 0 || <-retired != 1) {
        raise "owner deadline terminal cleanup";
    }
    sys->fprint(sys->fildes(2), "PASS original owner deadline: queued effect refused lifecycle export inert join\n");
}
