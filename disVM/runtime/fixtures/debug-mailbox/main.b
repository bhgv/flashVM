# Copyright (c) 2026 bhgv
# EN: Isolated owner diagnostics under real ordinary traffic, quota and retirement.
# RU: Отдельная owner diagnostics при ordinary traffic, quota и retirement.
implement DebugMailbox;
include "sys.m";
include "processctl.m";
DebugMailbox: module { init: fn(); };
Mode: con 0; # Private driver selects pending/retirement omission cases.
ctl: Processctl; # Immutable matched hosted interface.
sys: Sys; # Immutable scalar/formatting interface.


# EN/RU: Shared 48-byte reply with adjacent scalar canaries.
# Shared 48-byte reply с соседними scalar canaries.
Reply: adt
{
    before: int; # Leading canary.
    status: int; # Diagnostic status.
    owner: int; # Actual permanent owner PID, zero unless Ready.
    cookie: big; # Exact echoed scalar, zero unless Ready.
    dispatches: big; # Copied owner progress.
    turns: big; # Copied completed guest turns.
    waits: big; # Copied readiness waits.
    after: int; # Trailing canary.
};


# EN/RU: Hold this owner alive until parent termination, without executing another Prog.
# Сохранить owner до parent termination, без исполнения другого Prog.
hold()
{
    gate := chan of int;
    <-gate;
}


# EN/RU: Serve ordinary channel payloads; diagnostic wakeups must not consume them.
# Обслуживать ordinary channel payloads; diagnostics не поглощает их.
target(ready, gate, ack: chan of int)
{
    ready <-= ctl->pid();
    for (;;) {
        value := <-gate;
        ack <-= value;
    }
}


# EN/RU: Resolve only a held guest PID to its exact copied lifetime ID.
# Получить exact copied lifetime ID только удержанного guest PID.
identity(pid: int): big
{
    cursor := big 0;
    for (;;) {
        (status, id, found, done) := ctl->resident(cursor);
        if (status != 1 || id <= cursor) {
            raise "debug mailbox target identity missing";
        }
        if (found == pid) {
            return id;
        }
        cursor = id;
    }
}


# EN/RU: Wait until the target really blocks, not just before its first dispatch.
# Дождаться actual block target, а не только старта его first dispatch.
waitstate(id: big, activity: int)
{
    for (attempt := 0; attempt < 1000; attempt++) {
        (status, found) := ctl->residentactivity(id);
        if (status == 1 && found == activity) {
            return;
        }
        sys->sleep(1);
    }
    raise "debug mailbox target did not reach wait";
}


# EN: Poll this caller's ticket into shared fields; return zero only on bounded Pending.
# Ready validates exact owner/cookie and progress; every other status has zero payload.
# RU: Poll своего ticket в shared fields; ноль только при bounded Pending.
# Ready проверяет owner/cookie/progress; остальные statuses имеют zero payload.
receive(id, ticket: big, pid: int, cookie: big, r: ref Reply): int
{
    for (attempt := 0; attempt < 1000; attempt++) {
        (r.status, r.owner, r.cookie, r.dispatches, r.turns, r.waits) = ctl->residentreply(id, ticket);
        if (r.before != 12345 || r.after != 54321) {
            raise "debug mailbox reply canaries changed";
        }
        if (r.status == ctl->DiagReady) {
            if (r.owner != pid || r.cookie != cookie || r.turns > r.dispatches || r.dispatches <= big 0) {
                raise "debug mailbox wrong owner or copied reply";
            }
            return 1;
        }
        if (r.owner != 0 || r.cookie != big 0 || r.dispatches != big 0 || r.turns != big 0 || r.waits != big 0) {
            raise "debug mailbox non-ready payload not zero";
        }
        if (r.status != ctl->DiagPending) {
            raise "debug mailbox live ticket unexpectedly failed";
        }
        sys->sleep(1);
    }
    return 0;
}


# EN/RU: Exercise full signed-big cookie boundaries as well as ordinary values.
# Проверить signed-big cookie boundaries и обычные значения.
nonce(index: int): big
{
    if (index == 0) {
        return big "-9223372036854775808";
    }
    if (index == 7) {
        return big "9223372036854775807";
    }
    return big index - big 4;
}


# EN/RU: Other requester must not consume/forget this caller's existing ticket.
# Другой requester не должен поглощать/забывать ticket исходного caller.
thief(id, ticket: big, ready: chan of int)
{
    (status, pid, cookie, d, t, w) := ctl->residentreply(id, ticket);
    if (status != ctl->DiagDenied || pid != 0 || cookie != big 0 || d != big 0 || t != big 0 || w != big 0 ||
        ctl->residentforget(id, ticket) != ctl->DiagDenied) {
        raise "debug mailbox foreign requester stole ticket";
    }
    ready <-= ctl->pid();
    hold();
}


# EN/RU: Leave eight unconsumed tickets for automatic requester-retirement cleanup.
# Оставить восемь tickets для automatic requester-retirement cleanup.
leaker(id: big, ready: chan of int)
{
    for (index := 0; index < 8; index++) {
        (status, ticket) := ctl->residentrequest(id, big index);
        if (status != ctl->DiagReady || ticket <= big 0) {
            raise "debug mailbox leaker quota not accepted";
        }
    }
    ready <-= ctl->pid();
    hold();
}


# EN/RU: Keep requesting while parent exchanges ordinary payloads with the same target.
# Продолжать queries, пока parent обменивается ordinary payloads с тем же target.
inspector(id: big, pid: int, ready: chan of int)
{
    r := ref Reply(12345, 0, 0, big 0, big 0, big 0, big 0, 54321);
    for (index := 0; index < 32; index++) {
        (status, ticket) := ctl->residentrequest(id, -big index);
        if (status != ctl->DiagReady || !receive(id, ticket, pid, -big index, r)) {
            raise "debug mailbox traffic query failed";
        }
    }
    ready <-= ctl->pid();
    hold();
}


# EN/RU: Hold a genuine native lease until killed; owner cannot service it early.
# Удерживать actual native lease до kill; owner не может обслужить его раньше.
sleeper(ready: chan of int)
{
    ready <-= ctl->pid();
    sys->sleep(120000);
    raise "debug mailbox sleeper was not cancelled";
}


# EN/RU: Observe shared result publication continuously; stop/ready channels retain lifecycle.
# Непрерывно читать shared result; stop/ready channels удерживают lifecycle.
watcher(r: ref Reply, ready, stop: chan of int)
{
    ready <-= ctl->pid();
    reading := 1;
    while (reading) {
        if (r.before != 12345 || r.after != 54321 || r.status < -6 || r.status > 1 || r.owner < 0 ||
            r.dispatches < big 0 || r.turns < big 0 || r.waits < big 0) {
            raise "debug mailbox shared result changed";
        }
        alt {
            <-stop => reading = 0;
            * => sys->sleep(0);
        }
    }
    ready <-= ctl->pid();
    hold();
}


# EN/RU: Require actual direct-child retirement before reusing diagnostic quota.
# Требовать actual direct-child retirement до повторного использования quota.
retire(pid: int)
{
    if (ctl->killwait(pid) != ctl->Retired) {
        raise "debug mailbox child retirement refused";
    }
}


# EN: Test real owner replies, quotas, isolation, cancellation and all six joins.
# Private omission modes retire every created guest before precise DETECTED output.
# RU: Проверить owner replies, quota, isolation, cancellation и шесть joins.
# Private omission modes завершают всех созданных guests до DETECTED output.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Processctl Processctl->PATH;
    ready := chan of int;
    gate := chan of int;
    ack := chan of int;
    spawn target(ready, gate, ack);
    pid := <-ready;
    id := identity(pid);
    waitstate(id, ctl->ReceiveActivity);
    r := ref Reply(12345, 0, 0, big 0, big 0, big 0, big 0, 54321);
    if (Mode == 1) {
        (status, ticket) := ctl->residentrequest(id, big 77);
        if (status != ctl->DiagReady) {
            raise "debug pending control submit failed";
        }
        answered := receive(id, ticket, pid, big 77, r);
        ctl->residentforget(id, ticket);
        retire(pid);
        if (!answered) {
            sys->print("DETECTED debug mailbox wake ignored pending query\n");
        } else {
            sys->print("PASS debug mailbox pending control intact\n");
        }
        return;
    }
    if (Mode == 2) {
        spawn leaker(id, ready);
        leaking := <-ready;
        retire(leaking);
        (status, ticket) := ctl->residentrequest(id, big 88);
        leaked := status == ctl->DiagFull;
        if (status == ctl->DiagReady) {
            ctl->residentforget(id, ticket);
        }
        retire(pid);
        if (leaked) {
            sys->print("DETECTED debug mailbox requester quota was not released\n");
        } else {
            sys->print("PASS debug mailbox requester cleanup intact\n");
        }
        return;
    }
    stop := chan of int;
    spawn watcher(r, ready, stop);
    watching := <-ready;
    tickets := array[8] of big;
    for (index := 0; index < 8; index++) {
        (status, ticket) := ctl->residentrequest(id, nonce(index));
        if (status != ctl->DiagReady || ticket <= big 0) {
            raise "debug mailbox quota credit missing";
        }
        tickets[index] = ticket;
    }
    (status, ticket) := ctl->residentrequest(id, big 99);
    if (status != ctl->DiagFull || ticket != big 0) {
        raise "debug mailbox unbounded queue";
    }
    spawn thief(id, tickets[0], ready);
    retire(<-ready);
    for (index = 0; index < 8; index++) {
        if (!receive(id, tickets[index], pid, nonce(index), r)) {
            raise "debug mailbox held owner did not answer";
        }
    }
    (status, ticket) = ctl->residentrequest(id, big 123);
    if (status != ctl->DiagReady || ticket <= tickets[7] || ctl->residentforget(id, ticket) != ctl->DiagReady) {
        raise "debug mailbox forget or monotonic ticket failed";
    }
    (replystatus, owner, cookie, d, t, w) := ctl->residentreply(id, ticket);
    if (replystatus != ctl->DiagMissing || owner != 0 || cookie != big 0 || d != big 0 || t != big 0 || w != big 0) {
        raise "debug mailbox forgotten ticket aliased reply";
    }
    spawn leaker(id, ready);
    leaking := <-ready;
    (status, ticket) = ctl->residentrequest(id, big 0);
    if (status != ctl->DiagFull) {
        raise "debug mailbox leaker did not occupy credits";
    }
    retire(leaking);
    (status, ticket) = ctl->residentrequest(id, big "9223372036854775807");
    if (status != ctl->DiagReady || !receive(id, ticket, pid, big "9223372036854775807", r)) {
        raise "debug mailbox requester retirement leaked quota";
    }
    spawn inspector(id, pid, ready);
    for (index = 0; index < 32; index++) {
        gate <-= index + 100;
        if (<-ack != index + 100) {
            raise "debug mailbox changed ordinary channel traffic";
        }
    }
    retire(<-ready);
    spawn sleeper(ready);
    sleeping := <-ready;
    sleepid := identity(sleeping);
    waitstate(sleepid, ctl->NativeActivity);
    (status, ticket) = ctl->residentrequest(sleepid, big 17);
    if (status != ctl->DiagReady) {
        raise "debug mailbox native wait request refused";
    }
    (status, owner, cookie, d, t, w) = ctl->residentreply(sleepid, ticket);
    if (status != ctl->DiagPending || owner != 0 || cookie != big 0 || d != big 0 || t != big 0 || w != big 0 ||
        ctl->residentforget(sleepid, ticket) != ctl->DiagReady) {
        raise "debug mailbox serviced suspended native owner or failed cancel";
    }
    waitstate(sleepid, ctl->NativeActivity);
    (status, ticket) = ctl->residentrequest(sleepid, big 18);
    if (status != ctl->DiagReady) {
        raise "debug mailbox second native request refused";
    }
    retire(sleeping);
    (status, owner, cookie, d, t, w) = ctl->residentreply(sleepid, ticket);
    if ((status != ctl->DiagClosed && status != ctl->DiagMissing) || owner != 0 || cookie != big 0 ||
        d != big 0 || t != big 0 || w != big 0) {
        raise "debug mailbox retired target reply invalid";
    }
    (status, ticket) = ctl->residentrequest(big -1, big 0);
    if (status != ctl->DiagInvalid || ticket != big 0) {
        raise "debug mailbox invalid ID accepted";
    }
    (status, ticket) = ctl->residentrequest(big "9223372036854775807", big 0);
    if (status != ctl->DiagMissing || ticket != big 0) {
        raise "debug mailbox absent ID accepted";
    }
    stop <-= 1;
    if (<-ready != watching) {
        raise "debug mailbox watcher completion changed";
    }
    retire(watching);
    retire(pid);
    sys->print("PASS debug mailbox: owner replies bounded credits requester cleanup ordinary traffic native wait shared outputs joined\n");
}
