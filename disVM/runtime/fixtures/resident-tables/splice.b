# Copyright (c) 2026 bhgv
# EN: Opposing two-table transactions, alias splices and immutable slice validation.
# RU: Встречные транзакции двух таблиц, splice с алиасом и проверка снимков slice.
implement SpliceTables;
include "sys.m";
include "bigtable.m";

SpliceTables: module { init: fn(); };
sys: Sys; # Immutable interface initialized before threads start.
tables: Bigtable; # Immutable native table interface.
Workers: con 8; # Four residents in each opposite direction.
Rounds: con 64; # Three splice transactions and two snapshots per round.
Width: con 8; # A whole native transaction must preserve uniform payloads.


# EN: Validate a private copied snapshot, while peers keep replacing its source.
# snapshot belongs to the caller; its eight values must come from one writer.
# RU: Проверить личную копию, пока соседи продолжают менять источник.
# snapshot принадлежит вызывающему; восемь значений должны принадлежать одной записи.
check(snapshot: ref Bigtable->Handle)
{
    if (tables->length(snapshot) != big Width) {
        raise "two-table snapshot extent changed";
    }
    (status, first) := tables->getindex(snapshot, big 0);
    if (status != Bigtable->Ok || first < big 1 || first > big Workers) {
        raise "two-table snapshot writer changed";
    }
    for (at := 1; at < Width; at++) {
        (status, value) := tables->getindex(snapshot, big at);
        if (status != Bigtable->Ok || value != first) {
            raise "two-table snapshot contains a partial transaction";
        }
    }
}


# EN: Publish a private uniform replacement, then copy tables in opposing order.
# left/right/done are shared; id selects the private value. No table lease spans
# a source-language instruction, so ordinary contention must park rather than fail.
# RU: Создать личный однородный источник, затем копировать таблицы во встречном порядке.
# left/right/done общие; id задаёт личное значение. Lease не удерживается между
# инструкциями, поэтому обычная конкуренция должна ждать, а не ошибаться.
worker(left, right: ref Bigtable->Handle, id: int, done: chan of int)
{
    replacement := tables->create();
    for (at := 0; at < Width; at++) {
        tables->requireok(tables->setindex(replacement, big at, big (id + 1)));
    }
    for (round := 0; round < Rounds; round++) {
        tables->requireok(tables->splice(left, big 0, big Width, replacement));
        check(tables->slice(left, big 0, big Width));
        tables->requireok(tables->splice(right, big 0, big Width, left));
        check(tables->slice(right, big 0, big Width));
        tables->requireok(tables->splice(left, big 0, big Width, left));
    }
    done <-= 1;
}


# EN: Keep both direction groups resident and verify final guards are all empty.
# RU: Держать обе встречные группы и проверить снятие всех guard после завершения.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    left := tables->create();
    right := tables->create();
    for (at := 0; at < Width; at++) {
        tables->requireok(tables->setindex(left, big at, big 1));
        tables->requireok(tables->setindex(right, big at, big 1));
    }
    done := chan of int;
    for (id := 0; id < Workers; id++) {
        if (id % 2) {
            spawn worker(left, right, id, done);
        } else {
            spawn worker(right, left, id, done);
        }
    }
    for (id = 0; id < Workers; id++) {
        if (<-done != 1) {
            raise "two-table worker completion changed";
        }
    }
    check(left);
    check(right);
    (token, depth, waiting) := tables->guardstate(left);
    (other, levels, waiters) := tables->guardstate(right);
    if (token != big 0 || depth || waiting || other != big 0 || levels || waiters) {
        raise "two-table transaction retained a guard or waiter";
    }
    sys->print("PASS resident table splices transactions=1536 snapshots=1024 owners=8\n");
}
