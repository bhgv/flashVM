# Copyright (c) 2026 bhgv
# EN: Actual capture/info/mount/bind cancellation and committed-result races.
# RU: Настоящая отмена capture/info/mount/bind и гонки подтверждённого commit.
implement Check;
include "sys.m";
include "vfsexport.m";
include "vfsctl.m";
include "processctl.m";
Check: module { init: fn(); };
sys: Sys; # Immutable interface for permanent native guest owners.
api: Vfsexport; # Root capability capture/info/mount.
stack: Vfsctl; # Indexed bind bridge.
ctl: Processctl; # Direct-child authority.
root: ref Vfsexport->Root; # Shared immutable root capability through native pins.


# EN/RU: Fail an actual interpreter/JIT contract.
# Завершить actual interpreter/JIT contract при нарушении.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Select one exact native boundary after setup, before the actual operation.
# Выбрать точную native boundary после setup, перед настоящей операцией.
arm(mode: int)
{
    fd := sys->open("remote/arm", Sys->OWRITE);
    check(fd != nil && sys->write(fd, array[] of {byte mode}, 1) == 1, "arm acquire fixture");
}


# EN/RU: Each child owns one real operation; no event loop participates in cancellation.
# Каждый child владеет настоящей операцией; event loop не участвует в отмене.
child(kind: int, ready, done: chan of int)
{
    mode := 0;
    case kind {
    3 => mode = int 'T';
    4 => mode = int 'R';
    5 => mode = int 'V';
    7 or 8 or 10 => mode = int 'A';
    9 => mode = int 'I';
    }
    arm(mode);
    ready <-= ctl->pid();
    result := -1;
    case kind {
    0 or 6 =>
        path := "remote/hold";
        if (kind == 6) {
            path = "remote/ack";
        }
        captured := api->capture(path, 0, Vfsexport->Read|Vfsexport->Execute);
        check(captured == nil, "cancelled staged capture published a root");
    1 =>
        result = sys->bind("remote/hold", "target", 0);
    2 =>
        result = stack->bind("remote/hold", "target", Vfsctl->MREPL, 0);
    3 =>
        result = api->mount(root, "target", 0, 0);
    4 =>
        (result, nil, nil) = api->info(root);
    5 or 7 =>
        result = sys->bind("remote/fast", "target", 0);
    8 =>
        result = api->mount(root, "target", 0, 0);
    9 =>
        rights, directory: int;
        (result, rights, directory) = api->info(root);
        check(result == 0 && rights == 8r555 && directory == 1, "acknowledged root metadata changed");
    10 =>
        result = stack->bind("remote/fast", "target", Vfsctl->MREPL, 0);
    11 =>
        result = sys->bind("remote/metahold", "legacy", 0);
    }
    if (kind >= 7 && kind < 11) {
        check(result == 0, "committed namespace result lost to cancellation");
    } else {
        check(result == -1, "cancelled acquisition falsely succeeded");
    }
    done <-= result;
    never := chan of int;
    <-never;
}


# EN/RU: Observe native admission only; affected guest does not cooperate.
# Наблюдать только native admission; affected guest не кооперируется.
watch(fd: ref Sys->FD)
{
    bytes := array[1] of byte;
    check(sys->read(fd, bytes, 1) == 1 && bytes[0] == byte 'W', "native acquire admission");
}


# EN/RU: Read through live destination namespace; cancellation must preserve prior identity.
# Читать через live destination namespace; отмена должна сохранить прежнюю identity.
target(expected: int)
{
    fd := sys->open("target/item", Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1 && int bytes[0] == expected, "partial or lost namespace commit");
}


# EN/RU: Cancel/kill ten admitted operations, then test validation and commit races.
# Отменить/kill десять admitted operations, затем проверить validation и commit races.
init()
{
    sys = load Sys Sys->PATH;
    api = load Vfsexport Vfsexport->PATH;
    stack = load Vfsctl Vfsctl->PATH;
    stack->init();
    ctl = load Processctl Processctl->PATH;
    root = api->capture("remote", 0, Vfsexport->Read|Vfsexport->Execute);
    check(root != nil, "setup native root");
    observer := sys->open("remote/watch", Sys->OREAD);
    ready := chan of int;
    done := chan of int;
    for (terminate := 0; terminate < 2; terminate++) {
        for (kind := 0; kind < 5; kind++) {
            spawn child(kind, ready, done);
            pid := <-ready;
            watch(observer);
            if (terminate) {
                check(ctl->killwait(pid) == Processctl->Retired, "kill admitted acquisition");
            } else {
                check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == -1 &&
                      ctl->killwait(pid) == Processctl->Retired, "cancel admitted acquisition");
            }
            target(int 'T');
        }
    }
    for (edge := 5; edge < 12; edge++) {
        spawn child(edge, ready, done);
        pid := <-ready;
        watch(observer);
        expected := -1;
        if (edge >= 7 && edge < 11) {
            expected = 0;
        }
        check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == expected &&
              ctl->killwait(pid) == Processctl->Retired, "acquisition edge result");
        identity := int 'T';
        if (edge == 7 || edge >= 10) {
            identity = int 'C';
        }
        if (edge == 8 || edge == 9) {
            identity = int 'O';
        }
        target(identity);
    }
    sys->print("PASS guest VFS acquire wait\n");
}
