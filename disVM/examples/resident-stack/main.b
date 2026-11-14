# Copyright (c) 2026 bhgv
# EN: Inspect a real recursively blocked guest without pausing other native owners.
# RU: Исследовать реально рекурсивно заблокированного guest без pause других owners.
implement StackExample;
include "sys.m";
include "processctl.m";
include "residentstack.m";
StackExample: module { init: fn(); };


# EN/RU: Keep frames alive across a channel wait; parent joins this direct child.
# Сохранить frames во время channel wait; parent завершает direct child через join.
recurse(depth: int, entered: chan of int): int
{
    if (depth > 0) {
        return 1 + recurse(depth - 1, entered);
    }
    ctl := load Processctl Processctl->PATH;
    entered <-= ctl->pid();
    hold := chan of int;
    return <-hold;
}


# EN/RU: Start recursive worker; result use prevents tail-call frame elimination.
# Запустить recursive worker; использование result сохраняет non-tail frames.
worker(entered: chan of int)
{
    if (recurse(3, entered) < 0) {
        raise "stack example negative result";
    }
}


# EN: Resolve exact lifetime, assemble a complete caller-owned stack, then retire
# the target before printing. Display data must outlive its source thread.
# RU: Найти exact lifetime, собрать complete caller-owned stack, затем завершить
# target перед печатью. Display data должна переживать исходный поток.
init()
{
    ctl := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    debug := load Residentstack Residentstack->PATH;
    entered := chan of int;
    spawn worker(entered);
    pid := <-entered;
    cursor := big 0;
    for (;;) {
        (found, id, owner, done) := ctl->resident(cursor);
        if (found != 1) {
            raise "stack example missing resident";
        }
        cursor = id;
        if (owner == pid) {
            break;
        }
    }
    (status, stack) := debug->inspect(cursor, 1000);
    if (ctl->killwait(pid) != ctl->Retired) {
        raise "stack example target join refused";
    }
    if (status != ctl->DiagReady || stack == nil || stack.flags != ctl->StackComplete || len stack.frames < 4) {
        raise "stack example incomplete result";
    }
    for (index := 0; index < len stack.frames; index++) {
        frame := stack.frames[index];
        sys->print("frame %d: %s %s next/return PC %bd/%d flags=%d\n", index,
                   frame.name, frame.path, frame.pc, frame.instructions, frame.flags);
    }
    sys->print("PASS resident stack example\n");
}
