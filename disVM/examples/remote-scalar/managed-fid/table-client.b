# Copyright (c) 2026 bhgv
# EN: Verify ordinary load, table identity, native return and transactional key copy-back.
# RU: Проверить обычный load, идентичность таблиц, нативный результат и обратную запись ключей.


# EN: Use real local handles and one mounted module, with both VM heaps active.
# RU: Использовать настоящие локальные дескрипторы и один удалённый модуль с двумя heap.
check_tables(sys: Sys)
{
    remote := load Hybridcall "/transport/mounted/hybrid.dis";
    tables := load Bigtable Bigtable->PATH;
    State: import remote;
    if (remote == nil || tables == nil) {
        raise "load hybrid: " + sys->sprint("%r");
    }
    blank := remote->empty();
    if (blank == nil || tables->count(blank) != big 0) {
        raise "result-only table is not a native empty handle";
    }
    tab := seed_table(tables);
    second := tables->create();
    peers := array[] of {tab, second, nil};
    state := ref State(tab, 3);
    exports := load Vfsexport Vfsexport->PATH;
    projection := exports->table(tab);
    rejected := 0;
    {
        remote->apply(-1, tab, peers, state);
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected || state.stamp != 3 || tables->count(second) != big 0 || tables->length(tab) != big 2) {
        raise "failed call published a partial table or ADT update";
    }

    done := chan of (ref Bigtable->Handle, ref Bigtable->Handle, string);
    spawn call_tables(remote, tab, peers, state, done);
    sys->sleep(750);
    if (tables->settext(tab, "local-only", big 61) != tables->Ok ||
        tables->setindex(tab, big 8, big 62) != tables->Ok) {
        raise "could not update caller keys while provider waited";
    }
    (alias, newtab, problem) := <-done;
    if (problem != "" || alias != tab || state.source != tab || state.stamp != 19 || newtab == nil ||
        tables->length(tab) != big 1 || tables->count(tab) != big 8) {
        raise "table or ADT copy-back failed: " + problem;
    }
    (missing, ignored) := tables->getindex(tab, big 1);
    (as, added) := tables->getindex(tab, big 2);
    (ls, localindex) := tables->getindex(tab, big 8);
    (ss, sparse) := tables->getindex(tab, big 9223372036854775807);
    (ks, keyed) := tables->gettext(tab, "ключ😀");
    (cs, client) := tables->gettext(tab, "local-only");
    (os, other) := tables->gettext(second, "other");
    (fs, fresh) := tables->gettext(newtab, "returned");
    keys := tables->keysof(tab);
    (kind, number, last) := tables->keyat(keys, tables->keylen(keys) - big 1);
    if (missing != tables->Missing || as != tables->Ok || added != big 41 || ls != tables->Ok ||
        localindex != big 62 || ss != tables->Ok || sparse != big 7 || ks != tables->Ok || keyed != big 45 ||
        cs != tables->Ok || client != big 61 || os != tables->Ok || other != big 91 ||
        tables->length(second) != big 0 || tables->count(second) != big 1 ||
        fs != tables->Ok || fresh != -big 9223372036854775808 || tables->length(newtab) != big 1 ||
        kind != tables->Text || last != "1") {
        raise "numeric, keyed, ordered or unrelated entries lost in table copy-back";
    }
    length := exports->open(projection, ".len", Sys->OREAD);
    text := array[2] of byte;
    if (length == nil || sys->read(length, text, 2) != 2 || string text != "1\n") {
        raise "copy-back detached the existing VFS table projection";
    }
    check_table_rebinding(sys, remote, tables);
    sys->print("PASS managed Styx hybrid tables\n");
}


# EN: Seed both key namespaces with holes, UTF-8 and embedded zeroes.
# RU: Заполнить оба вида ключей, включая пропуски, UTF-8 и встроенный ноль.
seed_table(tables: Bigtable): ref Bigtable->Handle
{
    tab := tables->create();
    if (tables->setindex(tab, big 0, big 0) != tables->Ok ||
        tables->setindex(tab, big 1, big 19) != tables->Ok ||
        tables->setindex(tab, big 9223372036854775807, big 7) != tables->Ok ||
        tables->settext(tab, "1", big 29) != tables->Ok ||
        tables->settext(tab, "a\0б", big 31) != tables->Ok ||
        tables->settext(tab, "ключ😀", big 11) != tables->Ok) {
        raise "could not seed the hybrid table";
    }
    return tab;
}


# EN: Reject a rebound ref before applying either the table or the ADT updates.
# RU: Отклонить подменённую ссылку до записи таблиц и обновлений ADT.
check_table_rebinding(sys: Sys, remote: Hybridcall, tables: Bigtable)
{
    State: import remote;
    tab := seed_table(tables);
    second := tables->create();
    peers := array[] of {tab, second, nil};
    state := ref State(tab, 3);
    done := chan of (ref Bigtable->Handle, ref Bigtable->Handle, string);
    spawn call_tables(remote, tab, peers, state, done);
    sys->sleep(750);
    state.source = nil;
    (nil, nil, problem) := <-done;
    if (problem != "remote ref argument changed while waiting" || state.stamp != 3 ||
        state.source != nil || tables->length(tab) != big 2 || tables->count(second) != big 0) {
        raise "rebound table ref accepted a partial reply: " + problem;
    }
}


# EN: Report the result of the parallel call without risking a blocked receiver.
# RU: Передать результат параллельного вызова, чтобы принимающий не завис.
call_tables(remote: Hybridcall, tab: ref Bigtable->Handle, peers: array of ref Bigtable->Handle,
    state: ref Hybridcall->State, done: chan of (ref Bigtable->Handle, ref Bigtable->Handle, string))
{
    alias, fresh: ref Bigtable->Handle;
    info := "";
    {
        (alias, fresh) = remote->apply(1, tab, peers, state);
    } exception error {
        "*" => info = error;
    }
    done <-= (alias, fresh, info);
}
