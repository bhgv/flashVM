implement TableGuards;
include "sys.m";
include "bigtable.m";

TableGuards: module
{
    init: fn(); # Standalone native-cell entry; no GUI or remote transport required.
    failrange: fn(tables: Bigtable, tab: ref Bigtable->Handle); # Nested VM-frame cleanup probe.
};


# Verify normal/error paths release native payload guards before VM exceptions.
# All handles remain GC rooted. The token is diagnostic only, never supplied by this program.
init()
{
    sys := load Sys Sys->PATH;
    tables := load Bigtable Bigtable->PATH;
    tab := tables->create();
    if (tables->setindex(tab, big 0, big 42) != tables->Ok) {
        raise "fail: guarded table seed";
    }
    (owner, depth, waiters) := tables->guardstate(tab);
    if (owner != big 0 || depth != 0 || waiters != 0) {
        raise "fail: guard not released after store";
    }

    # The range check runs under the guard, but its exception is raised after leave.
    caught := 0;
    {
        failrange(tables, tab);
    } exception message {
    "invalid key/range" =>
        caught = 1;
    * =>
        raise message;
    }
    (owner, depth, waiters) = tables->guardstate(tab);
    if (!caught || owner != big 0 || depth != 0 || waiters != 0) {
        raise "fail: range exception stranded guard";
    }

    # A failed transactional splice returns a status; requireok then raises outside the guard.
    empty := tables->create();
    caught = 0;
    {
        tables->requireok(tables->splice(tab, big 0, big 2, empty));
    } exception message {
    "invalid key/range" =>
        caught = 1;
    * =>
        raise message;
    }
    (owner, depth, waiters) = tables->guardstate(tab);
    if (!caught || owner != big 0 || depth != 0 || waiters != 0) {
        raise "fail: splice exception stranded guard";
    }

    # Self-splice collapses the identical owner pair; snapshots have independent lifetimes.
    tables->requireok(tables->splice(tab, big 0, -big 1, tab));
    copy := tables->slice(tab, big 0, -big 1);
    keys := tables->keysof(tab);
    tab = nil;
    (status, value) := tables->getindex(copy, -big 1);
    if (status != tables->Ok || value != big 42 || tables->keylen(keys) != big 1) {
        raise "fail: guarded snapshot ownership";
    }
    (owner, depth, waiters) = tables->guardstate(copy);
    if (owner != big 0 || depth != 0 || waiters != 0) {
        raise "fail: copied owner not quiescent";
    }
    sys->print("PASS table guards, exception cleanup and independent snapshots\n");
}


# Raise from a builtin reached through an additional Limbo frame. Borrowed table
# remains rooted in both frames; unwinding must release neither a live guard nor
# the caller's remaining reference. The invalid range deliberately never returns.
failrange(tables: Bigtable, tab: ref Bigtable->Handle)
{
    tables->slicerange(tab, big 2, big 2);
}
