# Copyright (c) 2026 bhgv
# EN: Actual managed token aliases, quota, staging, foreign-thread refusal and close.
# RU: Настоящие managed token aliases, quota, staging, foreign-thread refusal и close.
implement ProxyLeaseNative;
include "sys.m";
include "proxygc.m";
ProxyLeaseNative: module { init: fn(); };
gc: Proxygc; # Immutable native descriptor interface.


# EN/RU: A foreign resident may read active state but cannot mutate/pop broker slots.
# Чужой resident читает active state, но не меняет/снимает broker slots.
foreign(connection: ref Proxygc->Connection, lease: ref Proxygc->Lease, done: chan of int)
{
    (session, unused, generation) := gc->next(connection);
    if (!gc->matches(lease, big 1, 0, big 1) || !gc->active(lease) || gc->consume(lease) || gc->activate(lease) || session != big 0) {
        raise "foreign native lease mutation admitted";
    }
    done <-= 1;
}


# EN/RU: Reserved results cannot bind, activate or allocate from a foreign resident.
# Reserved results нельзя bind/activate/allocate с чужого resident.
foreign_reserved(connection: ref Proxygc->Connection, lease: ref Proxygc->Lease, done: chan of int)
{
    if (gc->bind(lease, big 2, 0, big 2) || gc->activate(lease) ||
        gc->active(lease) || gc->reserve(connection) != nil) {
        raise "foreign result reservation mutation";
    }
    done <-= 1;
}


# EN/RU: Use real native token types and collector retirement, never a modeled finalizer.
# Использовать настоящие native token types и collector retirement, не modeled finalizer.
init()
{
    sys := load Sys Sys->PATH;
    gc = load Proxygc Proxygc->PATH;
    connection := gc->open(2);
    first := gc->make(connection, big 1, 0, big 1, 1);
    alias := first;
    staged := gc->make(connection, big 1, 1, big 1, 0);
    if (first == nil || staged == nil || gc->active(staged) ||
        gc->make(connection, big 1, 2, big 1, 1) != nil) {
        raise "native lease quota or staging";
    }
    if (!gc->matches(first, big 1, 0, big 1) ||
        gc->matches(first, big 2, 0, big 1) || gc->matches(first, big 1, 1, big 1) ||
        gc->matches(first, big 1, 0, big 2) || gc->matches(nil, big 1, 0, big 1) ||
        gc->matches(staged, big 1, 1, big 1)) {
        raise "native lease exact immutable owner match";
    }
    done := chan[1] of int;
    spawn foreign(connection, first, done);
    if (<-done != 1 || !gc->activate(staged) || gc->activate(staged) ||
        !gc->consume(staged) || gc->consume(staged)) {
        raise "native lease exact activation/release";
    }
    if (gc->matches(staged, big 1, 1, big 1)) {
        raise "consumed token still matched its old owner";
    }
    first = nil;
    sys->sleep(20);
    if (!gc->active(alias)) {
        raise "ordinary alias lost shared image ownership";
    }
    (session, slot, generation) := gc->next(connection);
    if (session != big 0) {
        raise "ordinary alias GC released live logical owner";
    }
    alias = nil;
    found := 0;
    for (attempt := 0; attempt < 1000 && !found; attempt++) {
        (session, slot, generation) = gc->next(connection);
        found = session == big 1 && slot == 0 && generation == big 1;
        sys->sleep(1);
    }
    if (!found) {
        raise "native collector did not enqueue last token";
    }
    next := gc->make(connection, big 2, 0, big 2, 1);
    if (next == nil) {
        raise "native pending slot quota was not recycled";
    }
    gc->close(connection);
    if (gc->active(next) || gc->matches(next, big 2, 0, big 2) || gc->make(connection, big 3, 0, big 1, 1) != nil) {
        raise "closed native queue revived";
    }
    results := gc->open(1);
    reserved := gc->reserve(results);
    if (reserved == nil || gc->active(reserved) || gc->activate(reserved) || gc->reserve(results) != nil) {
        raise "reserved result quota or premature activation";
    }
    spawn foreign_reserved(results, reserved, done);
    if (<-done != 1) {
        raise "foreign reserved result did not finish";
    }
    reserved = nil;
    for (attempt = 0; attempt < 1000 && reserved == nil; attempt++) {
        reserved = gc->reserve(results);
        sys->sleep(1);
    }
    if (reserved == nil || !gc->bind(reserved, big 2, 0, big 2) ||
        gc->bind(reserved, big 3, 0, big 3) || !gc->matches(reserved, big 2, 0, big 2) ||
        !gc->consume(reserved) || gc->bind(reserved, big 3, 0, big 3)) {
        raise "result reservation GC or exact once binding";
    }
    gc->close(results);
    sys->fprint(sys->fildes(2), "PASS native leases: aliases quota staging foreign refusal GC reuse close\n");
}
