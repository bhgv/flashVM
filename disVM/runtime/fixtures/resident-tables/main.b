# Copyright (c) 2026 bhgv
# EN: Concurrent scalar table calls with unique keys and optional retained leases.
# RU: Параллельные скалярные вызовы таблиц с личными ключами и удерживаемыми lease.
implement ResidentTables;
include "sys.m";
include "bigtable.m";

ResidentTables: module { init: fn(); };
sys: Sys; # Immutable Sys interface initialized before spawn.
tables: Bigtable; # Immutable table interface shared by permanent residents.
Workers: con 8; # Concurrent owners of distinct key ranges in one native table.
Count: con 128; # Unique keys per worker.
Faults: con 0; # Driver checks exceptions before and after payload execution.
Leased: con 0; # Driver also checks nested explicit/implicit guard levels.


# EN: Write/read unique keys; other residents concurrently grow the same payload.
# tab/done are shared; id owns one disjoint range. Report only after validation.
# RU: Писать/читать личные ключи при параллельном росте того же payload.
# tab/done общие; id владеет непересекающимся диапазоном. Отчёт после проверки.
worker(tab: ref Bigtable->Handle, id: int, done: chan of int)
{
    for (at := 0; at < Count; at++) {
        key := big (id * Count + at);
        value := key * big 17 + big 3;
        text := "ключ-λ-" + string key;
        if (Leased) {
            tables->acquire(tab);
            tables->acquire(tab);
        }
        tables->requireok(tables->setindex(tab, key, value));
        tables->requireok(tables->settext(tab, text, value));
        (status, found) := tables->getindex(tab, key);
        if (status != Bigtable->Ok || found != value) {
            raise "parallel table scalar value changed";
        }
        (status, found) = tables->gettext(tab, text);
        if (status != Bigtable->Ok || found != value) {
            raise "parallel table UTF-8 value changed";
        }
        if (at % 16 == 0) {
            keys := tables->keysof(tab);
            if (tables->keylen(keys) <= big 0) {
                raise "parallel table snapshot lost its own key";
            }
            if (Faults) {
                {
                    tables->getkey(tab, keys, big -1);
                    raise "invalid snapshot index was accepted";
                } exception {
                    "big table key index out of range" => {}
                }
                {
                    tables->slice(tab, big 0, big 2147483647);
                    raise "invalid table slice was accepted";
                } exception {
                    "invalid big table slice" => {}
                }
                (again, after) := tables->getindex(tab, key);
                if (again != Bigtable->Ok || after != value) {
                    raise "table did not recover after native exception";
                }
            }
        }
        if (Leased) {
            tables->release(tab);
            (token, depth, waiting) := tables->guardstate(tab);
            if (token == big 0 || depth != 1) {
                raise "primitive call changed the retained lease depth";
            }
            tables->release(tab);
        }
    }
    done <-= 1;
}


# EN: Require all distinct writes and complete same-table guard/FIFO retirement.
# RU: Проверить все независимые записи и снятие guard/FIFO той же таблицы.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    tab := tables->create();
    done := chan of int;
    for (id := 0; id < Workers; id++) {
        spawn worker(tab, id, done);
    }
    for (id = 0; id < Workers; id++) {
        if (<-done != 1) {
            raise "parallel table completion changed";
        }
    }
    if (tables->count(tab) != big (Workers * Count * 2) || tables->length(tab) != big (Workers * Count)) {
        raise "parallel table lost or duplicated keys";
    }
    for (at := 0; at < Workers * Count; at++) {
        (status, value) := tables->getindex(tab, big at);
        if (status != Bigtable->Ok || value != big at * big 17 + big 3) {
            raise "parallel table final value changed";
        }
        (status, value) = tables->gettext(tab, "ключ-λ-" + string at);
        if (status != Bigtable->Ok || value != big at * big 17 + big 3) {
            raise "parallel table final UTF-8 value changed";
        }
    }
    (token, depth, waiting) := tables->guardstate(tab);
    if (token != big 0 || depth || waiting) {
        raise "parallel table left an owner or waiter";
    }
    sys->print("PASS resident tables leased=%d keys=2048 owners=8\n", Leased);
}
