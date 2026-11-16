# Copyright (c) 2026 bhgv
# EN: Cancel/kill real Sys.chdir at metadata/search/capture/root boundaries and after commit.
# RU: Cancel/kill настоящего Sys.chdir на metadata/search/capture/root и после commit.
implement Check;
include "sys.m";
include "processctl.m";
Check: module { init: fn(); };
sys: Sys; # Immutable builtin operations.
ctl: Processctl; # Existing direct-child cancellation authority.
base: string; # Immutable native fixture virtual root, injected at compile time.


# EN/RU: Raise on an actual interpreter/JIT contract mismatch.
# Raise при нарушении actual interpreter/JIT contract.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Arm native boundary, then call unchanged Sys.chdir without an event loop.
# Установить native boundary, вызвать обычный Sys.chdir без event loop.
child(kind: int, ready, done: chan of int)
{
    modes := array[] of {byte 'M', byte 'X', byte 'C', byte 'I', byte 'K'};
    names := array[] of {"meta", "rights", "dir", "dir", "dir"};
    arm := sys->open(base+"/remote/arm", Sys->OWRITE);
    check(arm != nil && sys->write(arm, modes[kind:kind+1], 1) == 1, "arm native cwd");
    ready <-= ctl->pid();
    result := sys->chdir(base+"/remote/"+names[kind]);
    check(result == (kind == 4)-1, "cwd cancellation/commit result");
    done <-= result;
    never := chan of int;
    <-never;
}


# EN/RU: Observe native admission and cancel/kill only afterwards; no timing sleeps.
# Наблюдать native admission, затем cancel/kill без timing sleeps.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Processctl Processctl->PATH;
    base = "@FIXTURE_BASE@";
    observer := sys->open(base+"/remote/watch", Sys->OREAD);
    ready := chan of int;
    done := chan of int;
    bytes := array[1] of byte;
    for (terminate := 0; terminate < 2; terminate++) {
        for (kind := 0; kind < 4; kind++) {
            spawn child(kind, ready, done);
            pid := <-ready;
            check(sys->read(observer, bytes, 1) == 1 && bytes[0] == byte 'W', "cwd admission");
            if (terminate) {
                check(ctl->killwait(pid) == Processctl->Retired, "kill native cwd");
            } else {
                check(ctl->cancelwait(pid) == Processctl->Cancelled && <-done == -1 &&
                      ctl->killwait(pid) == Processctl->Retired, "cancel native cwd");
            }
            check(sys->open("plain/item", Sys->OREAD) != nil, "cancelled cwd changed namespace");
        }
    }
    spawn child(4, ready, done);
    pid := <-ready;
    check(sys->read(observer, bytes, 1) == 1 && ctl->cancelwait(pid) == Processctl->Cancelled &&
          <-done == 0 && ctl->killwait(pid) == Processctl->Retired, "committed cwd result lost");
    check(sys->open("plain/item", Sys->OREAD) == nil && sys->chdir(base) == 0 &&
          sys->open("plain/item", Sys->OREAD) != nil, "commit or native cwd restore");
    sys->print("PASS guest provider cwd wait\n");
}
