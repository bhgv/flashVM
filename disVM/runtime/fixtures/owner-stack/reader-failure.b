# Copyright (c) 2026 bhgv
# EN: Injected reader failure and Pending timeout must release their diagnostic credits.
# RU: Искусственная ошибка reader и Pending timeout должны освободить diagnostic credits.
implement ReaderFailure;
include "sys.m";
include "processctl.m";
include "residentstack.m";
ReaderFailure: module { init: fn(); };
ctl: Processctl; # Immutable matched diagnostic interface.
sys: Sys; # Immutable native scalar services.


# EN/RU: Publish PID, then wait on a channel or hold a cancellable native sleep.
# Сообщить PID, затем ждать канал либо удерживать cancellable native sleep.
worker(ready, gate: chan of int, sleeping: int)
{
    ready <-= ctl->pid();
    if (sleeping) {
        sys->sleep(120000);
    } else {
        <-gate;
    }
    raise "reader failure target unexpectedly resumed";
}


# EN/RU: Resolve a held child PID to its copied lifetime identity.
# Получить copied lifetime identity удержанного child PID.
identity(pid: int): big
{
    cursor := big 0;
    for (;;) {
        (status, id, found, done) := ctl->resident(cursor);
        if (status != 1) {
            raise "reader failure identity missing";
        }
        if (found == pid) {
            return id;
        }
        cursor = id;
    }
}


# EN/RU: Prove all eight credits are free after exception/timeout, then release them.
# Доказать доступность восьми credits после exception/timeout, затем освободить их.
quota(id: big)
{
    tickets := array[8] of big;
    for (index := 0; index < 8; index++) {
        (status, ticket) := ctl->residentrequest(id, big index);
        if (status != ctl->DiagReady) {
            raise "reader cleanup leaked diagnostic credit";
        }
        tickets[index] = ticket;
    }
    for (index = 0; index < 8; index++) {
        if (ctl->residentforget(id, tickets[index]) != ctl->DiagReady) {
            raise "reader cleanup quota release failed";
        }
    }
}


# EN: The driver installs a private library throwing after its byte-array allocation.
# Require exact rethrow, complete quota reuse and Pending cleanup without cancelling sleep.
# RU: Driver ставит private library с ошибкой после выделения byte-array. Проверить
# точный rethrow, quota reuse и Pending cleanup без отмены sleep.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Processctl Processctl->PATH;
    debug := load Residentstack Residentstack->PATH;
    ready := chan of int;
    gate := chan of int;
    spawn worker(ready, gate, 0);
    pid := <-ready;
    id := identity(pid);
    caught := 0;
    {
        debug->inspect(id, 1000);
    } exception {
        "stack reader injected failure" => caught = 1;
    }
    if (!caught) {
        raise "reader failure injection did not propagate";
    }
    quota(id);
    if (ctl->killwait(pid) != ctl->Retired) {
        raise "reader failure target retirement failed";
    }
    spawn worker(ready, gate, 1);
    pid = <-ready;
    id = identity(pid);
    waiting := 0;
    for (attempt := 0; attempt < 1000; attempt++) {
        (status, activity) := ctl->residentactivity(id);
        if (status == 1 && activity == ctl->NativeActivity) {
            waiting = 1;
            break;
        }
        sys->sleep(1);
    }
    if (!waiting) {
        raise "reader timeout target did not enter native wait";
    }
    (status, location) := debug->inspect(id, 1);
    if (status != ctl->DiagPending || location != nil) {
        raise "reader timeout fabricated a completed location";
    }
    quota(id);
    (after, activity) := ctl->residentactivity(id);
    if (after != 1 || activity != ctl->NativeActivity || ctl->killwait(pid) != ctl->Retired) {
        raise "reader timeout cancelled native work or leaked owner";
    }
    sys->print("PASS owner stack reader: managed error rethrow pending cleanup full quota native work preserved joined\n");
}
