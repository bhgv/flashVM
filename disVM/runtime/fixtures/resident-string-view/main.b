# Copyright (c) 2026 bhgv
# EN: Share Unicode native views, then test COW and cache refresh on private strings.
# RU: Общие Unicode native views, затем COW и обновление кэша личных строк.
implement StringViews;
include "sys.m";
include "processctl.m";
StringViews: module { init: fn(); };
sys: Sys; # Immutable builtin interface published before spawn.
control: Processctl; # Parent retires each permanent resident.
Workers: con 8; # Real OS owners of independent frames and environments.
Rounds: con 64; # Repeated reads of one shared native UTF-8 cache.


# EN: Read shared repeatedly; mutate a private COW alias and verify refreshed views.
# ready/start/done are borrowed channel endpoints; result carries the copied PID.
# RU: Многократно читать shared; менять личный COW-алиас и проверять свежие views.
# ready/start/done — заимствованные каналы; результат содержит скопированный PID.
worker(shared: string, ready, start, done: chan of int)
{
    ready <-= 1;
    <-start;
    for (at := 0; at < Rounds; at++) {
        sys->werrstr(shared);
        if (sys->sprint("%r") != shared) {
            raise "shared Unicode native view changed";
        }
    }
    own := shared;
    own[0] = '界';
    sys->werrstr(own);
    if (sys->sprint("%r") != own || shared[0] != 'Ж') {
        raise "native view damaged COW string ownership";
    }
    own[0] = '語';
    sys->werrstr(own);
    if (sys->sprint("%r") != own) {
        raise "private Unicode cache stayed stale after mutation";
    }
    own += "-more-語";
    sys->werrstr(own);
    if (sys->sprint("%r") != own) {
        raise "private Unicode cache failed to grow";
    }
    for (at = 0; at < 4; at++) {
        own[len own] = '界';
        sys->werrstr(own);
        if (sys->sprint("%r") != own) {
            raise "in-place Unicode append did not resize native cache";
        }
    }
    done <-= control->pid();
}


# EN: Publish one Unicode String to eight residents and wait for complete retirement.
# RU: Опубликовать одну Unicode String восьми потокам и дождаться завершения.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    shared := "Ж€界語🙂 native-view";
    for (id := 0; id < Workers; id++) {
        spawn worker(shared, ready, start, done);
    }
    for (id = 0; id < Workers; id++) {
        <-ready;
    }
    for (id = 0; id < Workers; id++) {
        start <-= 1;
    }
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "native string view owner did not retire";
        }
    }
    sys->print("PASS resident string views owners=8 shared_reads=512 cow=8 refresh=8 grow=8\n");
}
