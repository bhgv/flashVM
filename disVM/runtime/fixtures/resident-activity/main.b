# Copyright (c) 2026 bhgv
# EN: Real channel, table and native waits with copied activity and complete retirement.
# RU: Реальные channel, table и native waits с copied activity и полным retirement.
implement ResidentActivity;
include "sys.m";
include "processctl.m";
include "bigtable.m";
ResidentActivity: module { init: fn(); };
control: Processctl; # Immutable hosted diagnostic/control interface.
tables: Bigtable; # Shared table capability installed before spawn.


# EN: Find a held PID's copied lifetime ID; fail on absent or malformed registry.
# RU: Найти copied lifetime ID удержанного PID; ошибка при плохом/пустом registry.
identity(pid: int): big
{
    cursor := big 0;
    for (;;) {
        (status, id, found, done) := control->resident(cursor);
        if (status != 1 || id <= cursor) {
            raise "resident activity lost live PID";
        }
        if (found == pid) {
            return id;
        }
        cursor = id;
    }
}


# EN: Report Ready and PID on borrowed done after completion; borrowed pulse
# supplies rootless peer turns until parent kills this owner. Never returns normally.
# RU: Проверить Ready, отправить PID в borrowed done; borrowed pulse даёт peer turns
# до killwait родителя. Нормального возврата нет.
complete(done, pulse: chan of int)
{
    pid := control->pid();
    (status, activity) := control->residentactivity(identity(pid));
    if (status != 1 || activity != control->ReadyActivity) {
        raise "resident activity stale after wake";
    }
    done <-= pid;
    for (;;) {
        <-pulse;
    }
}


# EN: Publish PID/kind before blocking on a distinct unmatched channel or table.
# kind selects operation; ready reports kind/PID, gate supplies actual channel data,
# done reports completion, tab is a shared rooted table. Sleep is killwait-cancellable.
# RU: Сообщить PID/kind до ожидания на отдельном unmatched channel либо table.
# kind выбирает операцию; ready сообщает kind/PID, gate несёт channel data,
# done сообщает completion, tab — shared rooted table. Sleep отменяется killwait.
worker(kind: int, ready: chan of (int, int), gate: chan of int,
       done: chan of int, tab: ref Bigtable->Handle)
{
    ready <-= (kind, control->pid());
    case kind {
        0 => gate <-= 17;
        1 =>
            if (<-gate != 42) {
                raise "resident receive payload changed";
            }
        2 =>
            alt {
                value := <-gate =>
                    if (value != 73) {
                        raise "resident alt payload changed";
                    }
            }
        3 => tables->requireok(tables->setindex(tab, big 7, big 99));
        4 =>
            sys := load Sys Sys->PATH;
            sys->sleep(120000);
            raise "resident native sleep was not cancelled";
    }
    complete(done, gate);
}


# EN: Observe exact copied activities, wake actual operations, refuse invalid/gone IDs.
# No guessed pointers/foreign Prog reads; all children join and native ownership drains.
# RU: Наблюдать exact copied activities, будить actual operations, отказать invalid/gone ID.
# Без guessed pointers/чужих Prog reads; все children join и native ownership drains.
init()
{
    sys := load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    tables = load Bigtable Bigtable->PATH;
    tab := tables->create();
    tables->acquire(tab);
    ready := chan[5] of (int, int);
    done := chan[4] of int;
    gates := array[5] of chan of int;
    pids := array[5] of {* => 0};
    ids := array[5] of big;
    expected := array[] of {control->SendActivity, control->ReceiveActivity,
                           control->AltActivity, control->TableActivity, control->NativeActivity};
    for (index := 0; index < 5; index++) {
        kind := (index + 4) % 5;
        gates[kind] = chan of int;
        spawn worker(kind, ready, gates[kind], done, tab);
    }
    for (index = 0; index < 5; index++) {
        (kind, pid) := <-ready;
        if (kind < 0 || kind >= 5 || pids[kind]) {
            raise "resident activity duplicate or invalid ready";
        }
        pids[kind] = pid;
        ids[kind] = identity(pid);
    }
    seen := array[5] of {* => 0};
    count := 0;
    attempts := 0;
    while (count != 5 && attempts < 2000) {
        for (index = 0; index < 5; index++) {
            (status, activity) := control->residentactivity(ids[index]);
            if (status != 1) {
                raise "resident activity held identity disappeared";
            }
            if (!seen[index] && activity == expected[index]) {
                seen[index] = 1;
                count++;
            }
        }
        if (count != 5) {
            sys->sleep(1);
        }
        attempts++;
    }
    if (control->killwait(pids[4]) != control->Retired) {
        raise "resident native sleeper retirement refused";
    }
    gates[1] <-= 42;
    # Later IDs stay held: an exact-ID query must not alias the next sorted card.
    # Поздние ID удержаны: exact-ID query не должен подменяться следующей card.
    attempts = 0;
    for (;;) {
        (status, activity) := control->residentactivity(ids[4]);
        if (status == 0) {
            if (activity != 0) {
                raise "resident activity reaped ID leaked payload";
            }
            break;
        }
        if (status != 1 || attempts == 2000) {
            raise "resident activity reaped ID not retired";
        }
        # A real peer dispatch supplies rootless reaping progress; sleep alone
        # does not force this controller's current turn to finish.
        # Actual peer dispatch даёт reaping progress; sleep сам не завершает turn.
        gates[1] <-= 0;
        sys->sleep(1);
        attempts++;
    }
    if (<-gates[0] != 17) {
        raise "resident send payload changed";
    }
    gates[2] <-= 73;
    tables->release(tab);
    joined := array[4] of {* => 0};
    for (index = 0; index < 4; index++) {
        pid := <-done;
        found := -1;
        for (kind := 0; kind < 4; kind++) {
            if (pids[kind] == pid && !joined[kind]) {
                found = kind;
            }
        }
        if (found < 0) {
            raise "resident activity completion changed";
        }
        outcome := control->killwait(pid);
        if (outcome != control->Retired) {
            raise "resident activity retirement changed";
        }
        joined[found] = 1;
    }
    (tableStatus, value) := tables->getindex(tab, big 7);
    if (tableStatus != tables->Ok || value != big 99) {
        raise "resident observed table write did not complete";
    }
    for (index = -1; index <= 0; index++) {
        (status, activity) := control->residentactivity(big index);
        if (status != -1 || activity != 0) {
            raise "resident activity invalid ID leaked payload";
        }
    }
    (status, activity) := control->residentactivity(big "9223372036854775807");
    if (status != 0 || activity != 0) {
        raise "resident activity absent exact ID aliased another card";
    }
    if (count != 5) {
        sys->print("DETECTED resident activity missing send receive alt table native publication\n");
        return;
    }
    sys->print("PASS resident activity: send receive alt table native ready wake exact IDs reaped joined\n");
}
