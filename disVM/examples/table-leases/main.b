implement TableLeases;
include "sys.m";
include "bigtable.m";

TableLeases: module
{
    init: fn(); # Concurrent local acceptance entry.
    ordercheck: fn(tables: Bigtable, first: ref Bigtable->Handle, second: ref Bigtable->Handle): int;
    writer: fn(tables: Bigtable, tab: ref Bigtable->Handle, started: chan of int, done: chan of int);
    observer: fn(tables: Bigtable, tab: ref Bigtable->Handle, seen: chan of int);
    foreign: fn(tables: Bigtable, tab: ref Bigtable->Handle, done: chan of int);
    aborter: fn(tables: Bigtable, tab: ref Bigtable->Handle, started: chan of int, stop: chan of int, done: chan of int);
    retire: fn(tables: Bigtable, started: chan of int, stop: chan of int);
};


# Enter two recursive levels, suspend on channels and admit metadata diagnostics.
# The writer must park in MCALL without freezing the sole VM executor thread.
init()
{
    sys := load Sys Sys->PATH;
    tables := load Bigtable Bigtable->PATH;
    tab := tables->create();
    # Only one ordering of two distinct retained owners can be admitted.
    other := tables->create();
    rejected := ordercheck(tables, tab, other);
    rejected += ordercheck(tables, other, tab);
    if (rejected != 1) {
        raise "fail: retained lease ordering not enforced";
    }
    other = nil;
    tables->requireok(tables->setindex(tab, big 0, big 42));
    tables->acquire(tab);
    tables->acquire(tab);
    started := chan of int;
    done := chan of int;
    seen := chan of int;
    spawn writer(tables, tab, started, done);
    <-started;
    spawn observer(tables, tab, seen);
    if (<-seen != 1) {
        raise "fail: observer did not run while owner suspended";
    }
    spawn foreign(tables, tab, seen);
    if (<-seen != 1) {
        raise "fail: foreign release was admitted";
    }
    (status, value) := tables->getindex(tab, big 0);
    if (status != tables->Ok || value != big 42) {
        raise "fail: competing writer passed retained lease";
    }
    tables->release(tab);
    (token, depth, waiters) := tables->guardstate(tab);
    if (token == big 0 || depth != 1 || waiters != 1) {
        raise "fail: nested leave opened the payload";
    }
    tables->release(tab);
    <-done;
    (status, value) = tables->getindex(tab, big 0);
    if (status != tables->Ok || value != big 99) {
        raise "fail: parked writer did not retry";
    }

    # A caught application exception aborts this actor's leases before unwinding.
    stop := chan of int;
    spawn aborter(tables, tab, started, stop, done);
    <-started;
    spawn writer(tables, tab, started, done);
    <-started;
    spawn observer(tables, tab, seen);
    <-seen;
    stop <-= 1;
    <-done;
    <-done;
    (token, depth, waiters) = tables->guardstate(tab);
    if (token != big 0 || depth != 0 || waiters != 0) {
        raise "fail: exception stranded lease/waiter";
    }

    # Drop every application reference to a leased owner while the process waits.
    # Native root pinning must survive collector polls until normal process exit.
    spawn retire(tables, started, stop);
    <-started;
    for (i := 0; i < 100; i++) {
        tables->length(tab);
    }
    stop <-= 1;
    sys->print("PASS cooperative table leases, diagnostics, retries and abort cleanup\n");
}


# Report readiness, then request a conflicting store. The pending native frame
# remains rooted while parked; completion occurs only after final owner leave.
writer(tables: Bigtable, tab: ref Bigtable->Handle, started: chan of int, done: chan of int)
{
    started <-= 1;
    tables->requireok(tables->setindex(tab, big 0, big 99));
    done <-= 1;
}


# Inspect metadata without the payload lease. Bounded polling permits default
# scheduling as well as quantum 1; a frozen executor would never deliver seen.
observer(tables: Bigtable, tab: ref Bigtable->Handle, seen: chan of int)
{
    for (i := 0; i < 1000; i++) {
        (token, depth, waiters) := tables->guardstate(tab);
        if (token != big 0 && depth > 0 && waiters == 1) {
            seen <-= 1;
            return;
        }
    }
    raise "fail: cooperative waiter not visible";
}


# An unrelated process cannot unlock another actor, even while the owner waits.
foreign(tables: Bigtable, tab: ref Bigtable->Handle, done: chan of int)
{
    caught := 0;
    {
        tables->release(tab);
    } exception message {
    "table lease not owned" => caught = 1;
    * => raise message;
    }
    done <-= caught;
}


# Hold one owner through a channel wait, then raise into a local handler.
# Runtime cleanup must wake the competing writer before this function completes.
aborter(tables: Bigtable, tab: ref Bigtable->Handle, started: chan of int, stop: chan of int, done: chan of int)
{
    caught := 0;
    {
        tables->acquire(tab);
        started <-= 1;
        <-stop;
        raise "abort leased operation";
    } exception message {
    "abort leased operation" => caught = 1;
    * => raise message;
    }
    if (!caught) {
        raise "fail: application exception was not handled";
    }
    done <-= 1;
}


# Retain an otherwise unreferenced object in the lease registry, then intentionally
# omit release. Normal process retirement must retire that pin and physical guard.
retire(tables: Bigtable, started: chan of int, stop: chan of int)
{
    tab := tables->create();
    tables->acquire(tab);
    tab = nil;
    started <-= 1;
    <-stop;
}


# Probe one ordering without exposing native addresses. Return one on an order
# rejection, zero on successful paired acquire/release. Exceptions revoke both.
ordercheck(tables: Bigtable, first: ref Bigtable->Handle, second: ref Bigtable->Handle): int
{
    rejected := 0;
    {
        tables->acquire(first);
        tables->acquire(second);
        tables->release(second);
        tables->release(first);
    } exception message {
    "table lease order violation" => rejected = 1;
    * => raise message;
    }
    (owner, depth, waiters) := tables->guardstate(first);
    if (owner != big 0 || depth != 0 || waiters != 0) {
        raise "fail: lease ordering failure stranded ownership";
    }
    return rejected;
}
