# Copyright (c) 2026 bhgv
# EN: Retire a nested table lease owner while its child's cancel reply is held.
# RU: Завершить владельца вложенных lease при задержанном ответе отмены ребёнка.
implement TableParentExit;
include "sys.m";
include "bigtable.m";
include "processctl.m";

TableParentExit: module { init: fn(); };
sys: Sys; # Immutable interfaces published before resident creation.
tables: Bigtable; # Table remains rooted only by parent and child frames/leases.
control: Processctl; # Parent-scoped requests and late reply recipient lookup.
Writers: con 4; # FIFO successors survive their original parent's retirement.


# EN: Observe exact queue length on borrowed tab while this parent retains it.
# RU: Наблюдать точную длину очереди tab, пока родитель удерживает её lease.
queued(tab: ref Bigtable->Handle, expected: int)
{
    for (attempt := 0; attempt < 10000; attempt++) {
        (nil, nil, count) := tables->guardstate(tab);
        if (count == expected) {
            return;
        }
        sys->sleep(1);
    }
    raise "table parent exit queue did not converge";
}


# EN: Catch cancellation after the original parent has gone; frame roots preserve
# tab and snapshot through the held native completion. Verify the successors' final payload.
# ids/done are shared channels with copied PID and completion data only.
# RU: Поймать отмену после ухода родителя; корни кадра хранят tab и snapshot при задержанном
# native completion. Проверить итог записей преемников. ids/done передают только
# копии PID и данных завершения.
cancelled(tab: ref Bigtable->Handle, snapshot: ref Bigtable->Keys, ids, done: chan of int)
{
    ids <-= control->pid();
    {
        tables->acquire(tab);
        raise "cancelled orphan unexpectedly acquired its parent table";
    } exception {
        "table lease wait cancelled" => {}
    }
    (status, value) := tables->getindex(tab, big 0);
    keys := tables->keysof(tab);
    if (status != Bigtable->Ok || value != big Writers || tables->count(tab) != big 1 ||
            tables->keylen(keys) != big 1 || tables->keybig(keys, big 0) != big 0 ||
            tables->keylen(snapshot) != big 1 || tables->keybig(snapshot, big 0) != big 0) {
        raise "orphan table roots or FIFO payload changed after parent retirement";
    }
    (owner, depth, waiting) := tables->guardstate(tab);
    if (owner != big 0 || depth || waiting) {
        raise "retired table parent left a lease or waiter";
    }
    done <-= 1;
}


# EN: Commit id after its exact predecessor under one retained lease, then reply
# directly to the grandparent. tab/ids/done remain rooted by this orphan's frame.
# RU: Записать id после точного предшественника под lease и ответить деду.
# tab/ids/done укоренены кадром этого осиротевшего потока.
writer(tab: ref Bigtable->Handle, id: int, ids, done: chan of int)
{
    ids <-= control->pid();
    tables->acquire(tab);
    (status, previous) := tables->getindex(tab, big 0);
    if (status != Bigtable->Ok || previous != big (id - 1)) {
        raise "orphan table writer overtook its FIFO predecessor";
    }
    tables->requireok(tables->setindex(tab, big 0, big id));
    tables->release(tab);
    done <-= id;
}


# EN: Build three nested lease levels, queue five children and request cancellation
# of the head. The host delays its reply; this parent must never resume normally.
# ids/written/done outlive the parent. Only child frames retain tab after its death.
# RU: Создать три уровня lease, поставить пять детей и запросить отмену головы.
# Хост задерживает ответ; нормальное продолжение родителя запрещено. ids/written/
# done живут дольше родителя; после его смерти tab хранят только кадры детей.
parent(ids, written, done: chan of int)
{
    tab := tables->create();
    tables->requireok(tables->setindex(tab, big 0, big 0));
    snapshot := tables->keysof(tab);
    tables->acquire(tab);
    tables->acquire(tab);
    tables->acquire(tab);
    ids <-= control->pid();
    childids := chan of int;
    spawn cancelled(tab, snapshot, childids, done);
    target := <-childids;
    queued(tab, 1);
    for (id := 1; id <= Writers; id++) {
        spawn writer(tab, id, childids, written);
        <-childids;
        queued(tab, id + 1);
    }
    (owner, depth, count) := tables->guardstate(tab);
    if (owner == big 0 || depth != 3 || count != Writers + 1) {
        raise "table parent nested lease or queue changed before cancellation";
    }
    for (attempt := 0; attempt < 10000; attempt++) {
        result := control->cancelwait(target);
        if (result != Processctl->NotWaiting) {
            raise "table parent resumed before held cancellation reply was discarded";
        }
        sys->sleep(1);
    }
    raise "table parent cancellation never reached its queued child";
}


# EN: Wait for the genuine held completion, retire its parent and drain all FIFO
# successors before releasing it. No table reference is ever published to init.
# RU: Дождаться задержанного completion, завершить родителя и дождаться всех
# преемников FIFO до разрешения ответа. Ссылка на таблицу не публикуется в init.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    control = load Processctl Processctl->PATH;
    ids := chan of int;
    written := chan[Writers] of int;
    done := chan of int;
    spawn parent(ids, written, done);
    target := <-ids;
    entered := chan[101] of int;
    if (entered == nil || control->killwait(target) != Processctl->Retired ||
            control->killwait(target) != Processctl->Denied) {
        raise "table parent did not retire with its cancellation reply held";
    }
    seen := array[Writers + 1] of int;
    for (id := 0; id <= Writers; id++) {
        seen[id] = 0;
    }
    for (completed := 0; completed < Writers; completed++) {
        id = <-written;
        if (id < 1 || id > Writers || seen[id]) {
            raise "duplicate or invalid orphan table writer reply";
        }
        seen[id] = 1;
    }
    released := chan[102] of int;
    if (released == nil || <-done != 1) {
        raise "cancelled table orphan did not resume after parent retirement";
    }
    alt {
        <-written => raise "extra orphan table writer completion";
        * => {}
    }
    sys->print("PASS table parent exit levels=3 writers=4 owners=6 late_reply=1\n");
}
