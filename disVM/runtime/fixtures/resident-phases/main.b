# Copyright (c) 2026 bhgv
# EN: Observe real readiness waits and owner dispatch without reading foreign Prog state.
# RU: Наблюдать actual readiness waits и owner dispatch без чтения чужого Prog state.
implement ResidentPhases;
include "sys.m";
include "processctl.m";
ResidentPhases: module { init: fn(); };
control: Processctl; # Immutable hosted interface shared before spawn.


# EN: Return the caller's published phase; copied IDs never grant heap access.
# RU: Вернуть опубликованную phase вызывающего; copied IDs не дают heap access.
selfphase(): int
{
    own := control->pid();
    cursor := big 0;
    for (;;) {
        (status, id, pid, done, phase) := control->residentstate(cursor);
        if (status != 1 || id <= cursor) {
            raise "resident phase self disappeared";
        }
        if (pid == own) {
            if (done) {
                raise "resident phase self marked done";
            }
            return phase;
        }
        cursor = id;
    }
}


# EN: Report actual PID, block on release, then prove own callback is in Dispatch.
# RU: Сообщить actual PID, ждать release, затем доказать Dispatch своего callback.
worker(ready, release, done: chan of int)
{
    failed := selfphase() != control->Dispatch;
    ready <-= control->pid();
    <-release;
    if (selfphase() != control->Dispatch) {
        failed = 1;
    }
    pid := control->pid();
    if (failed) {
        pid = -pid;
    }
    done <-= pid;
    hold := chan of int;
    <-hold;
}


# EN: Return number of held known PIDs in WaitWork; refuse malformed/foreign cards.
# RU: Вернуть число удержанных известных PID в WaitWork; отказать плохим/чужим cards.
waiting(pids: array of int): int
{
    cursor := big 0;
    count := 0;
    waiting := 0;
    for (;;) {
        (status, id, pid, done, phase) := control->residentstate(cursor);
        if (status == 0) {
            if (id != big 0 || pid != 0 || done != 0 || phase != 0) {
                raise "resident phase end payload";
            }
            break;
        }
        if (status != 1 || id <= cursor || done || phase < control->Starting || phase > control->Teardown) {
            raise "resident phase live card invalid";
        }
        found := 0;
        for (index := 0; index < len pids; index++) {
            if (pids[index] == pid) {
                found++;
            }
        }
        if (found != 1) {
            raise "resident phase unknown PID";
        }
        if (phase == control->WaitWork) {
            waiting++;
        }
        cursor = id;
        count++;
    }
    if (count != len pids) {
        raise "resident phase lost held thread";
    }
    return waiting;
}


# EN: Hold eight real native guests until bounded repeated observation sees all waits.
# Join every guest after wake; invalid/end outputs are zero, not stale observations.
# RU: Удерживать восемь actual native guests до bounded наблюдения всех waits.
# Join каждого guest после wake; invalid/end outputs нулевые, не stale observations.
init()
{
    sys := load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan[8] of int;
    release := chan of int;
    done := chan[8] of int;
    for (index := 0; index < 8; index++) {
        spawn worker(ready, release, done);
    }
    pids := array[8] of int;
    for (index = 0; index < 8; index++) {
        pids[index] = <-ready;
    }
    attempts := 0;
    while (waiting(pids) != 8 && attempts < 2000) {
        sys->sleep(1);
        attempts++;
    }
    missingwait := attempts == 2000;
    (status, id, pid, retired, phase) := control->residentstate(big -1);
    if (status != -1 || id != big 0 || pid != 0 || retired != 0 || phase != 0) {
        raise "resident phase invalid cursor payload";
    }
    for (index = 0; index < 8; index++) {
        release <-= 1;
    }
    baddispatch := 0;
    for (index = 0; index < 8; index++) {
        child := <-done;
        if (child < 0) {
            baddispatch = 1;
            child = -child;
        }
        if (control->killwait(child) != control->Retired) {
            raise "resident phase child join refused";
        }
    }
    if (missingwait) {
        sys->print("DETECTED resident phases never published eight readiness waits\n");
        return;
    }
    if (baddispatch) {
        sys->print("DETECTED resident phases never published owner dispatch\n");
        return;
    }
    sys->print("PASS resident phases: eight waits dispatch before after wake joined\n");
}
