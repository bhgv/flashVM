# Copyright (c) 2026 bhgv
# EN: Release native tables/keys while two peers remain admitted to the same heap.
# RU: Освобождение таблиц/ключей при двух допущенных к той же куче соседях.
implement TableRetirement;
include "sys.m";
include "bigtable.m";
include "processctl.m";

TableRetirement: module { init: fn(); };
sys: Sys; # Immutable builtin interfaces published before spawn.
tables: Bigtable; # Native scalar tables with independent immutable key snapshots.
control: Processctl; # Parent waits for exact producer retirement.
Producers: con 4; # Independent native owner producers.
Rounds: con 64; # Each producer drops two tables and one keys object per round.


# EN: Keep an anchor lease/snapshot live through the native admitted-peer barrier.
# anchor/ready/done are rooted frame arguments; the host holds the real sleep(0).
# RU: Держать lease/снимок anchor в нативном барьере допущенного соседа.
# Аргументы укоренены кадром; хост задерживает настоящий sleep(0).
peer(anchor: ref Bigtable->Handle, ready, done: chan of int)
{
    tables->acquire(anchor);
    keys := tables->keysof(anchor);
    ready <-= 1;
    if (sys->sleep(0) != 0) {
        raise "table retirement peer sleep changed";
    }
    (status, value) := tables->getindex(anchor, big 0);
    if (status != Bigtable->Ok || value != big 99 || tables->keylen(keys) != big 1 ||
            tables->keybig(keys, big 0) != big 0) {
        raise "live table anchor or snapshot was reclaimed";
    }
    tables->release(anchor);
    done <-= 1;
}


# EN: Build and validate private native owners, then drop every managed reference.
# done reports the PID only after all Rounds native ownership transfers.
# RU: Создать/проверить личных нативных владельцев и снять все управляемые ссылки.
# done сообщает PID только после всех передач владения.
producer(done: chan of int)
{
    for (round := 0; round < Rounds; round++) {
        tab := tables->create();
        tables->acquire(tab);
        for (at := 0; at < 4; at++) {
            tables->requireok(tables->setindex(tab, big at, big (round * 4 + at)));
        }
        keys := tables->keysof(tab);
        copy := tables->slice(tab, big 0, big 4);
        tables->release(tab);
        if (tables->keylen(keys) != big 4 || tables->length(copy) != big 4) {
            raise "native owner construction changed";
        }
        for (at = 0; at < 4; at++) {
            (status, value) := tables->getindex(copy, big at);
            if (status != Bigtable->Ok || value != big (round * 4 + at)) {
                raise "native owner snapshot payload changed";
            }
        }
        tab = nil;
        copy = nil;
        keys = nil;
    }
    done <-= control->pid();
}


# EN: Keep only two peer anchors live, then require shared collection before release.
# The host sends stdin confirmation only after both peers stay admitted.
# RU: Оставить живыми только два anchor и потребовать сборку до снятия барьера.
# Хост подтверждает stdin только после допуска обоих соседей.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan of int;
    peers := chan of int;
    for (id := 0; id < 2; id++) {
        anchor := tables->create();
        tables->requireok(tables->setindex(anchor, big 0, big 99));
        spawn peer(anchor, ready, peers);
        <-ready;
        anchor = nil;
    }
    permit := array[1] of byte;
    input := sys->fildes(0);
    if (sys->read(input, permit, 1) != 1 || permit[0] != byte 49) {
        raise "admitted table peers were not confirmed";
    }
    done := chan of int;
    for (id = 0; id < Producers; id++) {
        spawn producer(done);
    }
    for (id = 0; id < Producers; id++) {
        pid := <-done;
        retired := control->killwait(pid);
        if (retired != Processctl->Retired && retired != Processctl->Denied) {
            raise "native table producer did not retire";
        }
    }
    sys->werrstr("shared native table retirement checkpoint");
    if (<-peers != 1 || <-peers != 1) {
        raise "admitted table peers did not recover";
    }
    sys->print("PASS native table retirement tables=512 keys=256 anchors=2 owners=6\n");
}
