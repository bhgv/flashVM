# Copyright (c) 2026 bhgv
# EN: Mutate two reconstructed tables in an original Dis export and return aliases.
# RU: Изменить две восстановленные таблицы в исходном экспорте Dis и вернуть алиасы.
implement Hybridcall;

include "sys.m";
include "managed-table.m";


# EN: Exercise a result-only signature with no table in the argument tuple.
# RU: Проверить сигнатуру с таблицей только в результате.
empty(): ref Bigtable->Handle
{
    tables := load Bigtable Bigtable->PATH;
    return tables->create();
}


# EN: Verify aliases, mutate both key spaces and optionally wait or fail.
# RU: Проверить алиасы, изменить оба вида ключей и при необходимости ждать или вызвать ошибку.
apply(mode: int, tab: ref Bigtable->Handle, peers: array of ref Bigtable->Handle,
    state: ref State): (ref Bigtable->Handle, ref Bigtable->Handle)
{
    tables := load Bigtable Bigtable->PATH;
    if (tab == nil || tab != peers[0] || tab != state.source || peers[2] != nil ||
        tables->length(tab) != big 2 || tables->count(tab) != big 6) {
        raise "table shape or aliases lost in request";
    }
    (ns, numeric) := tables->getindex(tab, big 1);
    (ts, textual) := tables->gettext(tab, "1");
    (bs, binary) := tables->gettext(tab, "a\0б");
    if (ns != tables->Ok || numeric != big 19 || ts != tables->Ok || textual != big 29 ||
        bs != tables->Ok || binary != big 31) {
        raise "typed or counted key lost in request";
    }
    fresh := tables->create();
    if (tables->delindex(tab, big 1) != tables->Ok ||
        tables->setindex(tab, big 2, big 41) != tables->Ok ||
        tables->deltext(tab, "1") != tables->Ok ||
        tables->settext(tab, "1", big 29) != tables->Ok ||
        tables->settext(tab, "ключ😀", big 45) != tables->Ok ||
        tables->settext(peers[1], "other", big 91) != tables->Ok ||
        tables->setindex(fresh, big 0, big 0) != tables->Ok ||
        tables->settext(fresh, "returned", -big 9223372036854775808) != tables->Ok) {
        raise "could not mutate native tables";
    }
    state.stamp = 19;
    if (mode < 0) {
        raise "intentional error after table mutations";
    }
    if (mode > 0) {
        table_barrier();
    }
    return (tab, fresh);
}


# EN: Leave time for an independent caller thread to update its own table.
# RU: Дать независимому потоку клиента изменить собственную таблицу.
table_barrier()
{
    sys := load Sys Sys->PATH;
    sys->sleep(2200);
}
