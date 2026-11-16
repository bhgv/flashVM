# Copyright (c) 2026 bhgv
# EN: Actual compound inspection cancellation, no stale replay and atomic layer moves.
# RU: Настоящая отмена compound inspection, отсутствие stale replay и atomic layer moves.
implement Check;
include "sys.m";
include "vfsctl.m";
include "processctl.m";
Check: module { init: fn(); };
sys: Sys; # Immutable builtin interface.
stack: Vfsctl; # Typed layer inspection/mutation wrapper.
ctl: Processctl; # Existing direct-child cancellation authority.


# EN/RU: Check real interpreter/JIT contracts in every mode.
# Проверить настоящие interpreter/JIT contracts во всех режимах.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Select a native move boundary after all setup/inspection calls.
# Выбрать native move boundary после setup/inspection calls.
arm(mode: int)
{
    fd := sys->open("remote/arm", Sys->OWRITE);
    check(fd != nil && sys->write(fd, array[] of {byte mode}, 1) == 1, "arm fixture");
}


# EN/RU: Execute a real inspection/move; affected guest has no cancellation event loop.
# Выполнить настоящий inspection/move без cancellation event loop affected guest.
child(kind: int, ready, done: chan of int)
{
    revision, id: big;
    if (kind >= 4 && kind <= 6) {
        (status, version, card) := stack->layer("stack/sub", big -1, 0);
        check(status == Vfsctl->Ready && card != nil, "setup inherited layer");
        revision = version;
        id = card.id;
        mode := int 'M';
        if (kind == 5) {
            mode = int 'V';
        }
        if (kind == 6) {
            mode = int 'K';
        }
        arm(mode);
    }
    ready <-= ctl->pid();
    result: int;
    if (kind < 4) {
        names := array[] of {"remote/meta", "remote/rights", "remote/second", "remote/ack"};
        (result, nil, card) := stack->layer(names[kind], big -1, 0);
        check(result == Vfsctl->Error && card == nil, "cancelled inspection published a partial card");
        done <-= result;
    } else if (kind <= 6) {
        result = stack->move("stack/sub", revision, id, -1);
        expected := Vfsctl->Error;
        if (kind == 6) {
            expected = 0;
        }
        check(result == expected, "move cancellation/commit outcome");
        done <-= result;
    } else {
        (result, nil, cards) := stack->layers("remote/meta");
        check(result == Vfsctl->Error && cards == nil, "cancelled stale inspection retried or published partial list");
        done <-= result;
    }
    never := chan of int;
    <-never;
}


# EN/RU: Observe only native admission, never poll the blocked guest.
# Наблюдать native admission, не опрашивать blocked guest.
watch(fd: ref Sys->FD)
{
    bytes := array[1] of byte;
    check(sys->read(fd, bytes, 1) == 1 && bytes[0] == byte 'W', "native admission");
}


# EN/RU: Verify live child/parent resolution and an unaffected later inspection.
# Проверить live child/parent resolution и независимый последующий inspection.
verify(childvalue: int)
{
    paths := array[] of {"stack/sub/item", "stack/item"};
    for (i := 0; i < len paths; i++) {
        fd := sys->open(paths[i], Sys->OREAD);
        bytes := array[1] of byte;
        expected := int 'A';
        if (i == 0) {
            expected = childvalue;
        }
        check(fd != nil && sys->read(fd, bytes, 1) == 1 && int bytes[0] == expected,
            "partial move or changed parent stack");
    }
    (status, nil, card) := stack->layer("remote/fast", big -1, 0);
    check(status == Vfsctl->Ready && card != nil && card.metadata == 0 && card.access == (5|(7<<8)),
        "later inspection inherited cancellation or invented permissions");
}


# EN/RU: Cancel/kill ten admissions and inspect commit/cancel/stale boundary races.
# Отменить/kill десять admissions, проверить commit/cancel/stale boundary races.
init()
{
    sys = load Sys Sys->PATH;
    stack = load Vfsctl Vfsctl->PATH;
    stack->init();
    ctl = load Processctl Processctl->PATH;
    check(sys->bind("remote", "stack", 0) == 0 &&
          sys->bind("lower", "stack", Sys->MAFTER) == 0, "setup two-source stack");
    observer := sys->open("remote/watch", Sys->OREAD);
    ready := chan of int;
    done := chan of int;
    for (terminate := 0; terminate < 2; terminate++) {
        for (kind := 0; kind < 5; kind++) {
            spawn child(kind, ready, done);
            pid := <-ready;
            watch(observer);
            if (terminate) {
                check(ctl->killwait(pid) == Processctl->Retired, "kill admitted inspection/move");
            } else {
                check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == Vfsctl->Error &&
                      ctl->killwait(pid) == Processctl->Retired, "cancel admitted inspection/move");
            }
            verify(int 'A');
        }
    }
    for (edge := 5; edge < 8; edge++) {
        spawn child(edge, ready, done);
        pid := <-ready;
        watch(observer);
        if (edge == 7) {
            check(sys->bind("remote", "changed", 0) == 0, "concurrent namespace revision");
        }
        expected := Vfsctl->Error;
        if (edge == 6) {
            expected = 0;
        }
        check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == expected &&
              ctl->killwait(pid) == Processctl->Retired, "inspection/move edge");
        value := int 'A';
        if (edge >= 6) {
            value = int 'B';
        }
        verify(value);
    }
    sys->print("PASS guest VFS inspection wait\n");
}
