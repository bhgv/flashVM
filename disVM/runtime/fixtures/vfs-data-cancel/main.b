# Copyright (c) 2026 bhgv
# EN: Actual admitted provider read/write/zero cancellation and killwait, through prepared views.
# RU: Actual admitted provider read/write/zero cancellation и killwait через prepared views.
implement Check;
include "sys.m";
include "vfsexport.m";
include "processctl.m";
Check: module { init: fn(); };
sys: Sys; # Immutable module reference shared by permanent guest threads.
api: Vfsexport; # Ordinary captured/prepared provider capabilities.
ctl: Processctl; # Caller-scoped direct-child cancellation authority.


# EN/RU: Explicit guest contract in interpreter and JIT.
# Явный guest contract в interpreter и JIT.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Signal PID, then perform real Sys I/O; native fixture announces actual admission separately.
# Передать PID и вызвать real Sys I/O; native fixture отдельно сообщает actual admission.
child(kind: int, shared: ref Sys->FD, ready, done: chan of int)
{
    names := array[] of {"alias", "view/write", "view/zero", "view/readn"};
    fd := shared;
    if (fd == nil) {
        mode := Sys->OREAD;
        if (kind == 1 || kind == 2) {
            mode = Sys->OWRITE;
        }
        fd = sys->open(names[kind], mode);
    }
    check(fd != nil, "open cancellable provider");
    ready <-= ctl->pid();
    bytes := array[2] of byte;
    result: int;
    if (kind == 0) {
        result = sys->read(fd, bytes, 1);
    } else if (kind == 3) {
        result = sys->readn(fd, bytes, 2);
        check(result == 1 && bytes[0] == byte 'A', "readn lost acknowledged prefix/replayed after cancellation");
    } else {
        count := 1;
        if (kind == 2) {
            count = 0;
        }
        result = sys->write(fd, bytes, count);
    }
    done <-= result;
    # Keep a completed test child alive so killwait observes a live direct child.
    # Сохранить completed test child живым для детерминированной проверки killwait.
    never := chan of int;
    <-never;
}


# EN/RU: Consume one native readiness record; this control does not poll the data worker.
# Прочитать одну native readiness record; control не опрашивает data worker.
watch(fd: ref Sys->FD)
{
    bytes := array[1] of byte;
    check(sys->read(fd, bytes, 1) == 1 && bytes[0] == byte 'R', "native admission record");
}


# EN/RU: Cancellation of one operation must leave a later fresh scope and prepared provider usable.
# Отмена одной операции сохраняет usable fresh scope и prepared provider.
fresh()
{
    fd := sys->open("view/fast", Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1 && bytes[0] == byte 'F', "later operation inherited cancellation");
}


# EN/RU: Exercise six admitted jobs and a shared-FD queued tail without guest cooperation.
# Проверить шесть admitted jobs и shared-FD queued tail без guest cooperation.
init()
{
    sys = load Sys Sys->PATH;
    api = load Vfsexport Vfsexport->PATH;
    ctl = load Processctl Processctl->PATH;
    root := api->capture("remote", 0, Vfsexport->Read|Vfsexport->Write|Vfsexport->Execute);
    prepared := api->overlay(root, nil, "/hidden", 0, Vfsexport->Tree);
    check(prepared != nil && api->mount(prepared, "view", 0, 0) == 0 &&
          sys->bind("view/read", "alias", 0) == 0, "prepare/bind cancellation view");
    observer := sys->open("remote/watch", Sys->OREAD);
    check(observer != nil, "native observer");
    ready := chan of int;
    done := chan of int;
    for (terminate := 0; terminate < 2; terminate++) {
        for (kind := 0; kind < 3; kind++) {
            spawn child(kind, nil, ready, done);
            pid := <-ready;
            watch(observer);
            if (terminate) {
                check(ctl->killwait(pid) == Processctl->Retired, "killwait admitted provider I/O");
            } else {
                cancelled := ctl->cancelwait(pid);
                value := <-done;
                retired := ctl->killwait(pid);
                check(cancelled == Processctl->Cancelled && value == -1 &&
                      retired == Processctl->Retired, "cancelwait admitted provider I/O");
            }
            fresh();
        }
    }
    spawn child(3, nil, ready, done);
    prefix := <-ready;
    watch(observer);
    check(ctl->cancelwait(prefix) == Processctl->Cancelled && <-done == 1 &&
          ctl->killwait(prefix) == Processctl->Retired, "readn cancellation prefix/retirement");
    shared := sys->open("alias", Sys->OREAD);
    spawn child(0, shared, ready, done);
    head := <-ready;
    watch(observer);
    spawn child(0, shared, ready, done);
    tail := <-ready;
    queued := sys->open("remote/queued", Sys->OREAD);
    watch(queued);
    check(ctl->cancelwait(tail) == Processctl->Cancelled && <-done == -1, "queued provider tail");
    check(ctl->cancelwait(head) == Processctl->Cancelled && <-done == -1, "tail cancelled admitted head");
    check(ctl->killwait(tail) == Processctl->Retired && ctl->killwait(head) == Processctl->Retired,
          "provider children did not retire");
    fresh();
    sys->print("PASS guest VFS data cancel read write zero killwait queued scopes\n");
}
