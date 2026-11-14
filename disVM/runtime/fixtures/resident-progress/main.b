# Copyright (c) 2026 bhgv
# EN: Observe increasing owner progress and coherent registry partitions during channel cycles.
# RU: Наблюдать рост owner progress и coherent registry partitions при channel cycles.
implement ResidentProgress;
include "sys.m";
include "processctl.m";
ResidentProgress: module { init: fn(); };
control: Processctl; # Immutable hosted interface installed before spawn.


# EN: Shared scalar outputs live through every worker join; canaries remain untouched.
# RU: Shared scalar outputs живут до join всех workers; canaries не меняются.
Result: adt
{
    before: int; # Leading canary.
    status: int; # Exact-ID query status.
    dispatches: big; # Copied callback entries.
    turns: big; # Copied completed guest callbacks.
    waits: big; # Copied readiness waits.
    after: int; # Trailing canary.
};


# EN: Shared registry output; observer readers compare individual fields only.
# RU: Shared registry output; readers сравнивают только отдельные fields.
Summary: adt
{
    before: int; # Leading canary.
    status: int; # Native summary availability.
    created: big; # Installed slots.
    registered: big; # Linked slots.
    reaping: big; # Detached reaper-owned slots.
    reaped: big; # Accounted joined slots.
    done: big; # Completed linked slots.
    after: int; # Trailing canary.
};


# EN: Borrow shared result; all scalar projections remain nonnegative during publication.
# RU: Заимствовать shared result; scalar projections неотрицательны при публикации.
checkshared(result: ref Result)
{
    if (result.before != 13579 || result.after != 97531 || result.status < -1 || result.status > 1 ||
        result.dispatches < big 0 || result.turns < big 0 || result.waits < big 0) {
        raise "resident progress shared scalar or canary corrupted";
    }
}


# EN: Send own PID on ready; each pulse/ack cycle forces a real channel wait/turn.
# result/gate/ack stay rooted; remain alive until parent killwait after four rounds.
# RU: Передать PID в ready; pulse/ack вызывает actual channel wait/turn.
# result/gate/ack rooted; жить до killwait родителя после четырёх rounds.
worker(result: ref Result, ready, gate, ack: chan of int)
{
    checkshared(result);
    ready <-= control->pid();
    for (round := 0; round < 4; round++) {
        if (<-gate != 100 + round) {
            raise "resident progress pulse changed";
        }
        checkshared(result);
        ack <-= control->pid();
    }
    for (;;) {
        <-gate;
    }
}


# EN: Resolve held PID to copied ID; refuse loss/invalid ordering without target reads.
# RU: Получить copied ID удержанного PID; отказать при потере/неверном порядке.
identity(pid: int): big
{
    cursor := big 0;
    for (;;) {
        (status, id, found, done) := control->resident(cursor);
        if (status != 1 || id <= cursor) {
            raise "resident progress identity missing";
        }
        if (found == pid) {
            return id;
        }
        cursor = id;
    }
}


# EN: Require coherent installed-slot partition; no child has exited yet here.
# RU: Проверить coherent installed-slot partition; здесь children ещё не выходили.
checksummary(s: ref Summary)
{
    (s.status, s.created, s.registered, s.reaping, s.reaped, s.done) = control->residentsummary();
    if (s.before != 24680 || s.after != 86420 || s.status != 1 || s.created != big 9 ||
        s.registered != big 9 || s.reaping != big 0 || s.reaped != big 0 || s.done != big 0 ||
        s.created != s.registered + s.reaping + s.reaped) {
        raise "resident progress registry summary inconsistent";
    }
}


# EN: Read shared progress/summary while parent publishes; stop only on command.
# ready reports PID; done reports read count, then this owner waits for killwait.
# RU: Читать shared progress/summary при публикациях родителя; stop по команде.
# ready сообщает PID; done — read count, затем owner ждёт killwait.
reader(result: ref Result, s: ref Summary, ready, stop, done: chan of int)
{
    sys := load Sys Sys->PATH;
    ready <-= control->pid();
    reading := 1;
    reads := 0;
    while (reading) {
        checkshared(result);
        if (s.before != 24680 || s.after != 86420 || s.status < -1 || s.status > 1 ||
            s.created < big 0 || s.registered < big 0 || s.reaping < big 0 || s.reaped < big 0 || s.done < big 0) {
            raise "resident summary shared scalar or canary corrupted";
        }
        reads++;
        alt {
            <-stop => reading = 0;
            * => sys->sleep(0);
        }
    }
    done <-= reads;
    hold := chan of int;
    <-hold;
}


# EN: Observe eight held owners through five increasing snapshots and all real joins.
# Only copied native counters are read; shared output fields are guarded by builtin.
# RU: Наблюдать восемь owners через пять растущих snapshots и actual joins.
# Читать только copied native counters; shared output fields guarded в builtin.
init()
{
    sys := load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan[8] of int;
    ack := chan[8] of int;
    gates := array[8] of chan of int;
    pids := array[8] of int;
    ids := array[8] of big;
    previous := array[8] of {* => big 0};
    oldturns := array[8] of {* => big 0};
    oldwaits := array[8] of {* => big 0};
    result := ref Result(13579, 0, big 0, big 0, big 0, 97531);
    summary := ref Summary(24680, 0, big 0, big 0, big 0, big 0, big 0, 86420);
    stop := chan of int;
    readerdone := chan of int;
    spawn reader(result, summary, ready, stop, readerdone);
    readerpid := <-ready;
    for (index := 0; index < 8; index++) {
        gates[index] = chan of int;
        spawn worker(result, ready, gates[index], ack);
        pids[index] = <-ready;
        ids[index] = identity(pids[index]);
    }
    failed := 0;
    for (round := 0; round <= 4; round++) {
        checksummary(summary);
        for (index = 0; index < 8; index++) {
            attempts := 0;
            for (;;) {
                if (failed) {
                    break;
                }
                (result.status, result.dispatches, result.turns, result.waits) = control->residentprogress(ids[index]);
                checkshared(result);
                if (result.status != 1 || result.turns > result.dispatches) {
                    raise "resident progress copied counters invalid";
                }
                if (result.dispatches > previous[index] && result.turns > oldturns[index] && result.waits > oldwaits[index]) {
                    break;
                }
                if (attempts == 1000) {
                    failed = 1;
                    break;
                }
                sys->sleep(1);
                attempts++;
            }
            previous[index] = result.dispatches;
            oldturns[index] = result.turns;
            oldwaits[index] = result.waits;
        }
        if (round < 4) {
            for (index = 0; index < 8; index++) {
                gates[index] <-= 100 + round;
            }
            seen := array[8] of {* => 0};
            for (index = 0; index < 8; index++) {
                pid := <-ack;
                found := -1;
                for (candidate := 0; candidate < 8; candidate++) {
                    if (pids[candidate] == pid && !seen[candidate]) {
                        found = candidate;
                    }
                }
                if (found < 0) {
                    raise "resident progress ack duplicated or foreign";
                }
                seen[found] = 1;
            }
        }
    }
    (status, d, t, w) := control->residentprogress(big -1);
    if (status != -1 || d != big 0 || t != big 0 || w != big 0) {
        raise "resident progress invalid ID payload";
    }
    (status, d, t, w) = control->residentprogress(big "9223372036854775807");
    if (status != 0 || d != big 0 || t != big 0 || w != big 0) {
        raise "resident progress absent ID payload";
    }
    for (index = 0; index < 8; index++) {
        if (control->killwait(pids[index]) != control->Retired) {
            raise "resident progress child retirement refused";
        }
    }
    stop <-= 1;
    if (<-readerdone <= 0 || control->killwait(readerpid) != control->Retired) {
        raise "resident observation reader did not run or retire";
    }
    if (failed) {
        sys->print("DETECTED resident progress stopped publishing completed turns\n");
        return;
    }
    sys->print("PASS resident progress: eight owners four wakes monotonic counters shared canaries summary joined\n");
}
