# Copyright (c) 2026 bhgv
# EN: Verify owner stack prefixes, imports, truncation, frozen tickets and safe projections.
# RU: Проверить owner stack prefixes, imports, truncation, frozen tickets и safe projections.
implement OwnerStack;
include "sys.m";
include "processctl.m";
include "residentstack.m";
include "provider.m";
OwnerStack: module { init: fn(); };
ctl: Processctl; # Immutable matched control/diagnostic module.
sys: Sys; # Immutable scalar services.


# EN/RU: Hold imported recursion, drop provider on return, then wait for parent join.
# Удерживать imported recursion, отпустить provider после return, ждать parent join.
worker(depth: int, ids, ready, gate, returned: chan of int)
{
    ids <-= ctl->pid();
    provider := load StackProvider "stack-λ.dis";
    if (provider == nil || provider->wait(depth, ready, gate) != 124 + depth) {
        raise "owner stack imported result corrupted";
    }
    provider = nil;
    returned <-= ctl->pid();
    hold := chan of int;
    <-hold;
}


# EN/RU: Resolve held guest PID to exact lifetime using copied registry cards.
# Найти exact lifetime удержанного guest PID по copied registry cards.
identity(pid: int): big
{
    cursor := big 0;
    for (;;) {
        (status, id, owner, done) := ctl->resident(cursor);
        if (status != 1) {
            raise "owner stack identity disappeared";
        }
        if (owner == pid) {
            return id;
        }
        cursor = id;
    }
}


# EN/RU: Poll summary without consuming its immutable credit; bound pending retries.
# Читать summary без consume immutable credit; ограничить pending retries.
header(id, ticket: big): (int, big, int, int)
{
    for (attempt := 0; attempt < 1000; attempt++) {
        (status, sequence, count, flags) := ctl->residentstack(id, ticket);
        if (status != ctl->DiagPending) {
            return (status, sequence, count, flags);
        }
        sys->sleep(1);
    }
    return (ctl->DiagPending, big 0, 0, 0);
}


# EN/RU: Copy one label; failure cannot produce an accepted partial string.
# Копировать одну подпись; отказ не может дать принятую partial string.
text(id, ticket: big, frame, part, length: int): string
{
    bytes := array[length] of byte;
    for (index := 0; index < length; index++) {
        (status, value) := ctl->residentframebyte(id, ticket, frame, part, index);
        if (status != ctl->DiagReady) {
            raise "owner stack label vanished";
        }
        bytes[index] = byte value;
    }
    return string bytes;
}


# EN/RU: Another requester must receive Denied and zero payload for all projections.
# Другой requester должен получить Denied и zero payload во всех projections.
thief(id, ticket: big, done: chan of int)
{
    (s, seq, count, flags) := ctl->residentstack(id, ticket);
    (f, fs, pc, extent, names, paths, bits) := ctl->residentframe(id, ticket, 0);
    (b, value) := ctl->residentframebyte(id, ticket, 0, 0, 0);
    if (s != ctl->DiagDenied || seq != big 0 || count || flags || f != ctl->DiagDenied || fs != big 0 ||
        pc != big 0 || extent || names || paths || bits || b != ctl->DiagDenied || value) {
        raise "owner stack foreign projection leaked";
    }
    done <-= ctl->pid();
    hold := chan of int;
    <-hold;
}


# EN: Check shallow complete/deep truncated imports; joins precede omission reports.
# Keep one ticket across return/drop, then prove scalar consumption and Missing.
# RU: Проверить shallow complete/deep truncated imports; joins до omission reports.
# Сохранить ticket через return/drop, доказать scalar consumption и Missing.
check(depth: int): int
{
    ids := chan of int;
    ready := chan of int;
    gate := chan of int;
    returned := chan of int;
    done := chan of int;
    spawn worker(depth, ids, ready, gate, returned);
    pid := <-ids;
    if (<-ready != 1) {
        raise "owner stack provider not entered";
    }
    id := identity(pid);
    (accepted, ticket) := ctl->residentrequest(id, big 81);
    if (accepted != ctl->DiagReady) {
        raise "owner stack request refused";
    }
    (status, sequence, count, flags) := header(id, ticket);
    expected := depth + 3;
    classification := ctl->StackComplete;
    if (expected > 8) {
        expected = 8;
        classification = ctl->StackTruncated;
    }
    good := status == ctl->DiagReady && sequence > big 0 && count == expected && flags == classification;
    labels := array[count] of string;
    pcs := array[count] of big;
    for (index := 0; index < count; index++) {
        (f, seq, pc, extent, names, paths, bits) := ctl->residentframe(id, ticket, index);
        name := text(id, ticket, index, 0, names);
        path := text(id, ticket, index, 1, paths);
        labels[index] = name + ":" + path;
        pcs[index] = pc;
        if (f != ctl->DiagReady || seq != sequence || pc < big 0 || pc > big extent || extent <= 0 ||
            !(bits & ctl->LocationValid)) {
            good = 0;
        }
        want := "StackProvider:stack-λ.dis";
        if (classification == ctl->StackComplete && index == count - 1) {
            want = "OwnerStack:main.dis";
        }
        if (labels[index] != want) {
            good = 0;
        }
    }
    spawn thief(id, ticket, done);
    if (ctl->killwait(<-done) != ctl->Retired) {
        raise "owner stack thief join failed";
    }
    (bad, bs, bp, bn, ba, bb, bf) := ctl->residentframe(id, ticket, count);
    (badbyte, value) := ctl->residentframebyte(id, ticket, 0, 2, 0);
    if (bad != ctl->DiagInvalid || bs != big 0 || bp != big 0 || bn || ba || bb || bf ||
        badbyte != ctl->DiagInvalid || value) {
        raise "owner stack bounds accepted";
    }
    gate <-= 123;
    if (<-returned != pid) {
        raise "owner stack permanent owner changed";
    }
    (frozen, seq2, n2, flags2) := header(id, ticket);
    if (frozen != status || seq2 != sequence || n2 != count || flags2 != flags) {
        raise "owner stack summary mutated after return";
    }
    for (index = 0; index < count; index++) {
        (f, seq, pc, extent, names, paths, bits) := ctl->residentframe(id, ticket, index);
        if (f != ctl->DiagReady || seq != sequence || pc != pcs[index] ||
            text(id, ticket, index, 0, names) + ":" + text(id, ticket, index, 1, paths) != labels[index]) {
            raise "owner stack frames mutated after return";
        }
    }
    debug := load Residentstack Residentstack->PATH;
    (fresh, stack) := debug->inspect(id, 1000);
    if (good && (fresh != ctl->DiagReady || stack == nil || stack.sequence <= sequence ||
        stack.flags != ctl->StackComplete || len stack.frames != 1 || stack.frames[0].name != "OwnerStack")) {
        raise "owner stack later caller boundary unavailable";
    }
    (reply, owner, cookie, d, t, w) := ctl->residentreply(id, ticket);
    (missing, ms, mn, mf) := ctl->residentstack(id, ticket);
    if (reply != ctl->DiagReady || owner != pid || cookie != big 81 || missing != ctl->DiagMissing ||
        ms != big 0 || mn || mf || ctl->killwait(pid) != ctl->Retired) {
        raise "owner stack ticket consumption or target join failed";
    }
    return good;
}


# EN/RU: Exercise both real stack depths and emit exact semantic results after all joins.
# Проверить обе настоящие глубины stack и выдать точный semantic result после joins.
init()
{
    ctl = load Processctl Processctl->PATH;
    sys = load Sys Sys->PATH;
    shallow := check(2);
    deep := check(80);
    if (!shallow || !deep) {
        sys->print("DETECTED owner stack missing or wrong copied caller chain\n");
        return;
    }
    sys->print("PASS owner stack: imports recursion growth truncation frozen caller frames ownership bounds joined\n");
}
