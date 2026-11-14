# Copyright (c) 2026 bhgv
# EN: Retired local caller cannot strand an alarm after the actual owner completes.
# RU: Retired local caller не оставляет alarm после completion actual owner.
implement OwnerOrphan;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
OwnerOrphan: module { init: fn(); };
proxy: Proxyowner; # Immutable original-owner library.


# EN/RU: Serve the original Cell with a completion-gated callback on this permanent Prog.
# Обслуживать original Cell с completion-gated callback на permanent Prog.
own(cell, capture: ref Proxyhost->Cell, done: chan of int,
    ready: chan of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle))
{
    handler := load Proxyhandler "owner-orphan-handler.dis";
    proxy->serve(handler, cell, capture, ready);
    done <-= 1;
}


# EN/RU: Offer one long-budget callback; the parent retires this caller after actual entry.
# Предложить callback с long budget; parent завершает caller после actual entry.
caller(endpoint: ref Proxyowner->Endpoint, variable, function: Proxyowner->Handle, started: chan of int)
{
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -777, 60000);
    raise "owner orphan caller resumed before retirement";
}


# EN/RU: Retire only the waiting caller, release accepted work and verify one effect/clean Stop.
# Завершить лишь waiting caller, отпустить accepted work, проверить один effect/clean Stop.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    proxy = load Proxyowner Proxyowner->PATH;
    proxy->init();
    cell := ref Proxyhost->Cell(10);
    capture := ref Proxyhost->Cell(0);
    ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    done := chan[1] of int;
    spawn own(cell, capture, done, ready);
    (endpoint, variable, function) := <-ready;
    started := chan[1] of int;
    spawn caller(endpoint, variable, function, started);
    child := <-started;
    deadline := sys->millisec() + 15000;
    gate := sys->open("owner-orphan-entered", Sys->OREAD);
    while (gate == nil && sys->millisec() < deadline) {
        sys->sleep(5);
        gate = sys->open("owner-orphan-entered", Sys->OREAD);
    }
    if (gate == nil || control->killwait(child) != Processctl->Retired) {
        raise "owner orphan entry or caller retirement";
    }
    gate = nil;
    release := sys->create("owner-orphan-release", Sys->OWRITE, 8r666);
    if (release == nil) {
        raise "owner orphan release";
    }
    release = nil;
    (status, value) := proxy->call(endpoint, Proxyowner->Read, variable, variable, variable, 0, 0, 10000);
    (stopped, unused) := proxy->call(endpoint, Proxyowner->Stop, variable, variable, variable, 0, 0, 10000);
    if (status != 0 || value != 11 || cell.value != 11 || stopped != 0 || <-done != 1) {
        raise "owner orphan effect replay or retirement leak";
    }
    sys->fprint(sys->fildes(2), "PASS owner orphan: accepted effect once retired caller alarm joined\n");
}
