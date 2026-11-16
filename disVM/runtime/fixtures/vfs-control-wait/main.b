# Copyright (c) 2026 bhgv
# EN: Real guest path/open/held-metadata cancellation and acknowledged-open preservation.
# RU: Real guest path/open/held-metadata cancellation и сохранение acknowledged open.
implement Check;
include "sys.m";
include "vfsexport.m";
include "vfsctl.m";
include "processctl.m";
Check: module { init: fn(); };
sys: Sys; # Immutable interface shared by permanent native guest owners.
api: Vfsexport; # Ordinary prepared-view composition.
ctl: Processctl; # Existing direct-child authority only.


# EN/RU: Explicit test contract in interpreter/JIT.
# Явный test contract в interpreter/JIT.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Arm a held-fid operation after every setup/open has finished.
# Arm held-fid operation после завершения setup/open.
arm()
{
    fd := sys->open("remote/arm", Sys->OWRITE);
    check(fd != nil && sys->write(fd, array[] of {byte 'A'}, 1) == 1, "arm native fixture");
}


# EN/RU: Each child executes one actual Sys operation; readiness is observed natively.
# Каждый child выполняет actual Sys operation; readiness наблюдается native-кодом.
child(kind: int, ready, done: chan of int)
{
    fd: ref Sys->FD;
    if (kind >= 2 && kind <= 4) {
        names := array[] of {"view/fstat", "view/seek", "view/dir"};
        fd = sys->open(names[kind-2], Sys->OREAD);
        check(fd != nil, "setup held metadata FD");
        arm();
    }
    ready <-= ctl->pid();
    result: int;
    case kind {
    0 =>
        (result, nil) = sys->stat("view/path");
    1 =>
        fd = sys->open("view/open", Sys->OREAD);
        result = -1;
        check(fd == nil, "cancelled unacknowledged open published FD");
    2 or 4 =>
        (result, nil) = sys->fstat(fd);
    3 =>
        result = int sys->seek(fd, big 0, Sys->SEEKEND);
    5 =>
        fd = sys->open("view/openack", Sys->OREAD);
        check(fd != nil, "acknowledged open lost to cancellation");
        result = 0;
    6 =>
        (result, nil) = sys->stat("union/missing");
    9 =>
        (result, nil) = sys->stat("view/statack");
        check(result == 0, "acknowledged stat was repeated/lost after cancellation");
    8 =>
        fd = sys->create("create/missing", Sys->OWRITE, 8r666);
        check(fd == nil, "cancelled lookup created a resource");
        result = -1;
    7 =>
        fd = sys->open("auto/missing", Sys->OREAD);
        check(fd == nil, "cancelled auto union opened a lower provider");
        result = -1;
    }
    done <-= result;
    # Keep the completed child live for deterministic killwait/resource checks.
    # Сохранить completed child живым для детерминированного killwait/resource check.
    never := chan of int;
    <-never;
}


# EN/RU: Observe one native admission record; this never polls the affected guest.
# Наблюдать native admission record без опроса affected guest.
watch(fd: ref Sys->FD)
{
    bytes := array[1] of byte;
    check(sys->read(fd, bytes, 1) == 1 && bytes[0] == byte 'R', "native control admission");
}


# EN/RU: Cancel/kill ten real control calls, then check acknowledgements and union stop rules.
# Отменить/kill десять real control calls, затем acknowledgement и union stop rules.
init()
{
    sys = load Sys Sys->PATH;
    api = load Vfsexport Vfsexport->PATH;
    ctl = load Processctl Processctl->PATH;
    root := api->capture("remote", 0, Vfsexport->Read|Vfsexport->Write|Vfsexport->Execute);
    view := api->overlay(root, nil, "/hidden", 0, Vfsexport->Tree);
    check(view != nil && api->mount(view, "view", 0, 0) == 0, "prepared control view");
    check(sys->bind("remote", "union", 0) == 0 && sys->bind("lower", "union", Sys->MAFTER) == 0 &&
          sys->bind("remote", "auto", 0) == 0 &&
          sys->bind("lower", "auto", Sys->MAFTER|Vfsctl->MAUTORWX) == 0, "two-provider control stacks");
    observer := sys->open("remote/watch", Sys->OREAD);
    ready := chan of int;
    done := chan of int;
    for (terminate := 0; terminate < 2; terminate++) {
        for (kind := 0; kind < 5; kind++) {
            spawn child(kind, ready, done);
            pid := <-ready;
            watch(observer);
            if (terminate) {
                check(ctl->killwait(pid) == Processctl->Retired, "killwait admitted control operation");
            } else {
                check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == -1 &&
                      ctl->killwait(pid) == Processctl->Retired, "cancelwait admitted control operation");
            }
            (status, nil) := sys->stat("view/fast");
            check(status == 0, "later path inherited cancellation");
        }
    }
    for (kind := 5; kind < 10; kind++) {
        spawn child(kind, ready, done);
        pid := <-ready;
        watch(observer);
        check(ctl->cancelwait(pid) == Processctl->Cancelled, "control edge cancellation");
        expected := -1;
        if (kind == 5 || kind == 9) {
            expected = 0;
        }
        check(<-done == expected && ctl->killwait(pid) == Processctl->Retired, "control acknowledgement/fallback");
    }
    sys->print("PASS guest VFS control wait open stat fstat seek cancellation acknowledgement union\n");
}
