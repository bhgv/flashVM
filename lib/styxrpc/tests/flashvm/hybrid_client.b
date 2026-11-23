# Copyright (c) 2026 bhgv
# EN: Accept FlashVM table patches through ordinary typed load.
# RU: Приём патчей таблиц FlashVM через обычный типизированный load.
implement Client;
include "sys.m";
include "hybrid.m";

Client: module { init: fn(); };


# EN: Seed numeric, sparse and counted text keys with overlapping spellings.
# RU: Заполнить числовые, разреженные и строковые ключи с совпадающими написаниями.
seed(tables: Bigtable): ref Bigtable->Handle
{
    tab := tables->create();
    if (tables->setindex(tab, big 0, big 0) != tables->Ok ||
        tables->setindex(tab, big 1, big 19) != tables->Ok ||
        tables->setindex(tab, big 9223372036854775807, big 7) != tables->Ok ||
        tables->settext(tab, "1", big 29) != tables->Ok ||
        tables->settext(tab, "a\0б", big 31) != tables->Ok ||
        tables->settext(tab, "after", big 33) != tables->Ok) {
        raise "could not seed table";
    }
    return tab;
}


# EN: Verify changes, aliases, fresh results, exact integers and text ordering.
# RU: Сверить изменения, алиасы, новые результаты, точные числа и порядок ключей.
init()
{
    sys := load Sys Sys->PATH;
    tables := load Bigtable Bigtable->PATH;
    graph := load Hybrid "/rpc/mounted/flash-hybrid.dis";
    if (graph == nil || tables == nil) {
        raise sys->sprint("cannot load hybrid exports: %r");
    }
    Bag: import graph;
    tab := seed(tables);
    inner := tables->create();
    bag := ref Bag(tab, inner, 3);
    peers := array[] of {tab, nil};
    blank := graph->empty();
    if (blank == nil || tables->count(blank) != big 0) {
        raise "invalid result-only table";
    }
    rejected := 0;
    {
        graph->reject(tab, peers, bag);
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected || bag.stamp != 3 || tables->count(inner) != big 0 || tables->length(tab) != big 2) {
        raise "remote error committed table changes";
    }
    (returned, fresh) := graph->change(tab, peers, bag);
    (absent, ignored) := tables->getindex(tab, big 1);
    (appended, added) := tables->getindex(tab, big 2);
    (sparse, far) := tables->getindex(tab, big 9223372036854775807);
    (utf, counted) := tables->gettext(tab, "a\0б");
    (keyed, text) := tables->gettext(tab, "1");
    (nested, value) := tables->gettext(inner, "updated");
    (created, whole) := tables->gettext(fresh, "returned");
    keys := tables->keysof(tab);
    (kind, ordinal, moved) := tables->keyat(keys, tables->keylen(keys) - big 2);
    if (returned != tab || bag.source != tab || bag.inner != inner || bag.stamp != 19 ||
        fresh == tab || absent != tables->Missing || appended != tables->Ok || added != big 41 ||
        sparse != tables->Ok || far != big 7 || utf != tables->Ok || counted != big 31 ||
        keyed != tables->Ok || text != big 29 || nested != tables->Ok || value != big 91 ||
        created != tables->Ok || whole != -big 9223372036854775808 ||
        kind != tables->Text || moved != "1" || tables->length(tab) != big 1 || tables->count(tab) != big 7) {
        raise "table graph copy-back or order mismatch";
    }
    sys->print("PASS DisVM loaded FlashVM hybrid tables\n");
}
