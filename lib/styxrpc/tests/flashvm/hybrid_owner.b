# Copyright (c) 2026 bhgv
# EN: Use real Bigtable handles for direct and nested table edits from the FlashVM caller.
# RU: Настоящие Bigtable для изменений прямых и вложенных таблиц по вызову FlashVM.
implement Hybrid;
include "hybrid.m";


# EN: Return a result-only table export.
# RU: Вернуть таблицу из профиля только с результатом.
empty(): ref Bigtable->Handle
{
    tables := load Bigtable Bigtable->PATH;
    return tables->create();
}


# EN: Check aliases, mutate both tables and return the original and a new one.
# RU: Сверить алиасы, изменить обе таблицы и вернуть исходную и новую.
change(tab: ref Bigtable->Handle, peers: array of ref Bigtable->Handle,
    bag: ref Bag): (ref Bigtable->Handle, ref Bigtable->Handle)
{
    tables := load Bigtable Bigtable->PATH;
    (ni, numeric) := tables->getindex(tab, big 1);
    (ti, textual) := tables->gettext(tab, "1");
    (bi, counted) := tables->gettext(tab, "a\0б");
    if (tab != bag.source || peers[0] != tab || peers[1] != nil ||
        ni != tables->Ok || numeric != big 19 || ti != tables->Ok || textual != big 29 ||
        bi != tables->Ok || counted != big 31) {
        raise "incorrect snapshot keys or identity";
    }
    if (tables->delindex(peers[0], big 1) != tables->Ok ||
        tables->setindex(tab, big 2, big 41) != tables->Ok ||
        tables->deltext(tab, "1") != tables->Ok ||
        tables->settext(tab, "1", big 29) != tables->Ok ||
        tables->settext(tab, "new", big 52) != tables->Ok ||
        tables->settext(bag.inner, "updated", big 91) != tables->Ok) {
        raise "could not change table";
    }
    bag.stamp = 19;
    fresh := tables->create();
    if (tables->settext(fresh, "returned", -big 9223372036854775808) != tables->Ok) {
        raise "could not create result table";
    }
    return (tab, fresh);
}


# EN: Exercise error handling after changing both private table copies.
# RU: Проверить ошибку после изменения обеих частных копий.
reject(tab: ref Bigtable->Handle, peers: array of ref Bigtable->Handle,
    bag: ref Bag): (ref Bigtable->Handle, ref Bigtable->Handle)
{
    change(tab, peers, bag);
    raise "discard table changes";
}
