# Copyright (c) 2026 bhgv
# EN: Actual admitted directory cancellation and policy-checked batch publication.
# RU: Настоящая отмена dirread после допуска и публикация проверенного batch.
implement Check;
include "sys.m";
include "vfsexport.m";
include "processctl.m";
Check: module { init: fn(); };
sys: Sys; # Immutable Sys interface; no cross-thread managed pointers in native callbacks.
api: Vfsexport; # Prepared-view composition through ordinary providers.
ctl: Processctl; # Direct-child cancellation authority.


# EN/RU: Fail the actual interpreter/JIT fixture on a violated contract.
# Завершить actual interpreter/JIT fixture при нарушении contract.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Open before readiness; arm native observation then perform one real dirread.
# Открыть до readiness; включить native observation, затем выполнить настоящий dirread.
child(kind: int, ready, done: chan of int)
{
    names := array[] of {"remote/dir", "view/dir", "union", "remote/ack", "view/prefix"};
    fd := sys->open(names[kind], Sys->OREAD);
    check(fd != nil, "open cancellable directory");
    arm := sys->open("remote/arm", Sys->OWRITE);
    check(arm != nil && sys->write(arm, array[] of {byte 'A'}, 1) == 1, "arm fixture");
    ready <-= ctl->pid();
    (count, entries) := sys->dirread(fd);
    if (kind >= 3) {
        check(count == 1 && entries != nil && len entries == 1 && entries[0].name == "first",
            "acknowledged or policy-checked directory prefix lost");
    } else {
        check(count == -1 && entries == nil, "cancelled directory became EOF");
    }
    (again, nil) := sys->dirread(fd);
    check(again == -1, "interrupted directory silently resumed");
    done <-= count;
    never := chan of int;
    <-never;
}


# EN/RU: Observe a native method admission; no polling or cooperation by affected guest.
# Наблюдать native admission без polling или cooperation affected guest.
watch(fd: ref Sys->FD)
{
    bytes := array[1] of byte;
    check(sys->read(fd, bytes, 1) == 1 && bytes[0] == byte 'R', "native directory admission");
}


# EN/RU: Hold only setup until parent changes namespace, then perform real guarded dirread.
# Задержать только setup до изменения namespace родителем, затем выполнить actual dirread.
guarded(kind: int, ready, start, done: chan of int)
{
    path := "dynamic";
    if (kind) {
        path = "remote/empty";
    }
    fd := sys->open(path, Sys->OREAD);
    check(fd != nil, "open before late legacy bind");
    ready <-= ctl->pid();
    <-start;
    control := sys->open("remote/arm", Sys->OWRITE);
    check(control != nil && sys->write(control, array[] of {byte 'A'}, 1) == 1, "arm legacy callback counter");
    (count, entries) := sys->dirread(fd);
    check(count == -1 && entries == nil &&
          sys->sprint("%r") == "directory provider lacks cancellation support; reopen directory",
          "late legacy bind entered or became EOF");
    control = sys->open("remote/disarm", Sys->OWRITE);
    check(control != nil && sys->write(control, array[] of {byte 'D'}, 1) == 1, "disarm legacy counter");
    done <-= 1;
    never := chan of int;
    <-never;
}


# EN/RU: Exercise plain/view/union cancellation, kill and acknowledged prefixes.
# Проверить plain/view/union cancellation, kill и подтверждённые prefix.
init()
{
    sys = load Sys Sys->PATH;
    api = load Vfsexport Vfsexport->PATH;
    ctl = load Processctl Processctl->PATH;
    root := api->capture("remote", 0, Vfsexport->Read|Vfsexport->Write|Vfsexport->Execute);
    view := api->overlay(root, nil, "/hidden", 0, Vfsexport->Tree);
    check(view != nil && api->mount(view, "view", 0, 0) == 0, "prepared directory view");
    check(sys->bind("remote", "union", 0) == 0 &&
          sys->bind("remote", "union", Sys->MAFTER) == 0, "two-provider directory stack");
    observer := sys->open("remote/watch", Sys->OREAD);
    ready := chan of int;
    done := chan of int;
    for (terminate := 0; terminate < 2; terminate++) {
        for (kind := 0; kind < 3; kind++) {
            spawn child(kind, ready, done);
            pid := <-ready;
            watch(observer);
            if (terminate) {
                check(ctl->killwait(pid) == Processctl->Retired, "kill admitted dirread");
            } else {
                check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == -1 &&
                      ctl->killwait(pid) == Processctl->Retired, "cancel admitted dirread");
            }
            (status, nil) := sys->stat("view/first");
            check(status == 0, "directory scope survived into next call");
        }
    }
    for (kind := 3; kind < 5; kind++) {
        spawn child(kind, ready, done);
        pid := <-ready;
        watch(observer);
        check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == 1 &&
              ctl->killwait(pid) == Processctl->Retired, "publish acknowledged directory prefix");
    }
    check(sys->bind("remote/empty", "dynamic", 0) == 0 &&
          sys->bind("remote/empty", "dynamic", Sys->MAFTER) == 0, "initial dynamic union");
    start := chan of int;
    for (late := 0; late < 2; late++) {
        spawn guarded(late, ready, start, done);
        pid := <-ready;
        target := "dynamic";
        flags := Sys->MAFTER;
        if (late) {
            target = "remote/empty/graft";
            flags = 0;
        }
        check(sys->bind("legacy", target, flags) == 0, "late legacy bind");
        start <-= 1;
        check(<-done == 1 && ctl->killwait(pid) == Processctl->Retired, "guarded directory completion");
    }
    sys->print("PASS guest VFS directory wait\n");
}
