# Copyright (c) 2026 bhgv
# EN: Edit table FIFO head/middle/tail while independent resident families contend.
# RU: Изменять голову/середину/хвост FIFO таблиц при конкуренции независимых семей.
implement TableFifoEdit;
include "sys.m";
include "bigtable.m";
include "processctl.m";

TableFifoEdit: module { init: fn(); };
sys: Sys; # Immutable interface initialized before spawn.
tables: Bigtable; # Per-table native storage and FIFO metadata.
control: Processctl; # Parent-scoped control requests and correlated replies.
Parents: con 4; # Independent concurrently active FIFO owners.
Rounds: con 4; # Fresh table and eight child threads per family round.
Workers: con 8; # Ordered original admissions; six survive queue edits.


# EN: Observe exact FIFO occupancy on borrowed tab; sleep yields only the OS.
# RU: Наблюдать точную длину FIFO заимствованной tab; sleep уступает только ОС.
queued(tab: ref Bigtable->Handle, expected: int)
{
    for (attempt := 0; attempt < 10000; attempt++) {
        (nil, nil, count) := tables->guardstate(tab);
        if (count == expected) {
            return;
        }
        sys->sleep(1);
    }
    raise "edited table FIFO occupancy did not converge";
}


# EN: Enter the table or acknowledge one cancellation and await explicit retry.
# id/previous define expected payload order, not OS completion order. Every frame
# roots tab/channels until exit; cancelled replies preserve this resident's PID.
# RU: Войти в таблицу либо подтвердить одну отмену и ждать разрешения повтора.
# id/previous задают порядок данных, не завершения ОС. Кадр хранит tab/каналы
# до выхода; ответ отмены содержит PID того же закреплённого потока.
writer(tab: ref Bigtable->Handle, id, previous: int, ids, cancelled, retry, done: chan of int)
{
    ids <-= control->pid();
    {
        tables->acquire(tab);
    } exception {
        "table lease wait cancelled" => {
            if (id != 3 && id != 7) {
                raise "unexpected FIFO writer cancellation";
            }
            cancelled <-= control->pid();
            <-retry;
            tables->acquire(tab);
        }
    }
    (status, last) := tables->getindex(tab, big 0);
    if (status != Bigtable->Ok || last != big previous) {
        raise "table FIFO survivor or retry overtook its predecessor";
    }
    length := tables->length(tab);
    tables->requireok(tables->setindex(tab, length, big id));
    tables->requireok(tables->setindex(tab, big 0, big id));
    tables->release(tab);
    done <-= id;
}


# EN: Join a direct child after its last acknowledgement; natural retirement may
# already have finished. pid is a copied identity, never another thread's context.
# RU: Дождаться прямого ребёнка после последнего ответа; естественный выход мог
# уже завершиться. pid — копия идентификатора, не контекст чужого потока.
retired(pid: int)
{
    result := control->killwait(pid);
    if (result != Processctl->Retired && result != Processctl->Denied) {
        raise "FIFO child retirement reply changed";
    }
}


# EN: Preserve one retained parent lease while cancelling middle/tail and killing
# middle/head. Retry removed writers at the tail, then audit the exact payload log.
# done reports this family's completion only after all child retirements.
# RU: Держать lease родителя при отмене середины/хвоста и kill середины/головы.
# Повторить снятых в хвосте и проверить точный журнал данных. done сообщает
# завершение семьи только после завершения всех её детей.
family(done: chan of int)
{
    for (round := 0; round < Rounds; round++) {
        tab := tables->create();
        tables->requireok(tables->setindex(tab, big 0, big -1));
        tables->acquire(tab);
        (owner, nil, nil) := tables->guardstate(tab);
        ids := chan of int;
        cancelled := chan of int;
        reports := chan[Workers] of int;
        retries := array[Workers] of chan of int;
        pids := array[Workers] of int;
        previous := array[] of {-99, -1, 1, 6, 2, -99, 4, 3};
        order := array[] of {1, 2, 4, 6, 3, 7};
        for (id := 0; id < Workers; id++) {
            retries[id] = chan of int;
            spawn writer(tab, id, previous[id], ids, cancelled, retries[id], reports);
            pids[id] = <-ids;
            queued(tab, id + 1);
        }
        if (control->cancelwait(pids[3]) != Processctl->Cancelled || <-cancelled != pids[3]) {
            raise "middle FIFO cancellation lost resident identity";
        }
        queued(tab, 7);
        if (control->cancelwait(pids[7]) != Processctl->Cancelled || <-cancelled != pids[7]) {
            raise "tail FIFO cancellation lost resident identity";
        }
        queued(tab, 6);
        if (control->killwait(pids[5]) != Processctl->Retired) {
            raise "middle FIFO kill did not retire";
        }
        queued(tab, 5);
        if (control->killwait(pids[0]) != Processctl->Retired) {
            raise "head FIFO kill did not retire";
        }
        queued(tab, 4);
        retries[3] <-= 1;
        queued(tab, 5);
        retries[7] <-= 1;
        queued(tab, 6);
        (after, depth, count) := tables->guardstate(tab);
        if (after != owner || depth != 1 || count != 6 || tables->length(tab) != big 1) {
            raise "FIFO edits changed retained parent lease or wrote early";
        }
        tables->release(tab);
        seen := array[Workers] of int;
        for (id = 0; id < Workers; id++) {
            seen[id] = 0;
        }
        for (completed := 0; completed < len order; completed++) {
            id = <-reports;
            if (id < 0 || id >= Workers || id == 0 || id == 5 || seen[id]) {
                raise "duplicate or forbidden FIFO writer completion";
            }
            seen[id] = 1;
        }
        for (id = 0; id < Workers; id++) {
            if (id != 0 && id != 5) {
                retired(pids[id]);
            }
            if (control->kill(pids[id]) != Processctl->Denied ||
                    control->cancelwait(pids[id]) != Processctl->Denied) {
                raise "retired FIFO writer accepted another request";
            }
        }
        (after, depth, count) = tables->guardstate(tab);
        if (after != big 0 || depth || count || tables->length(tab) != big 7 || tables->count(tab) != big 7) {
            raise "edited table FIFO did not completely drain";
        }
        for (at := 0; at < len order; at++) {
            (status, value) := tables->getindex(tab, big (at + 1));
            if (status != Bigtable->Ok || value != big order[at]) {
                raise "table FIFO payload log differs from survivor order";
            }
        }
        alt {
            <-reports => raise "extra table FIFO completion";
            * => {}
        }
    }
    done <-= 1;
}


# EN: Keep independent FIFO edits concurrent; wait for all borrowed done replies.
# RU: Выполнять независимые изменения FIFO параллельно и собрать ответы done.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    control = load Processctl Processctl->PATH;
    done := chan of int;
    for (id := 0; id < Parents; id++) {
        spawn family(done);
    }
    for (id = 0; id < Parents; id++) {
        if (<-done != 1) {
            raise "table FIFO family did not finish";
        }
    }
    sys->print("PASS table FIFO edits families=4 rounds=16 writers=128 order=1,2,4,6,3,7\n");
}
