# Copyright (c) 2026 bhgv
# EN: Exercise real owner variables/functions, aliases, deadlines and terminal cleanup.
# RU: Проверить реальные owner variables/functions, aliases, deadlines и terminal cleanup.
implement ProxyOwnerTest;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";

ProxyOwnerTest: module { init: fn(); };
proxy: Proxyowner; # Immutable shared library initialized before spawn.
Clients: con 8; # Actual concurrently executing client Progs.
Rounds: con 4; # Calls per client, serialized only by this variable's owner.


# EN/RU: Exercise one shared function image from this permanent client Prog.
# Вызывать общий function image с этого постоянного client Prog.
client(endpoint: ref Proxyowner->Endpoint, variable, function: Proxyowner->Handle, done: chan of int)
{
    for (index := 0; index < Rounds; index++) {
        (status, value) := proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, 1, 5000);
        if (status != 0 || value <= 0) {
            raise "concurrent proxy callback failed";
        }
    }
    done <-= 1;
}


# EN/RU: Wait for a deliberately blocking callback, then report unknown completion.
# Дождаться намеренно blocked callback, затем сообщить unknown completion.
blocked(endpoint: ref Proxyowner->Endpoint, variable, function: Proxyowner->Handle, done: chan of int)
{
    (status, unused) := proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -999, 1000);
    done <-= status;
}


# EN/RU: Call with a deadline while the callback waits for the original-cell gate.
# Вызвать с deadline, пока callback ждёт gate в исходной cell.
delayed(endpoint: ref Proxyowner->Endpoint, variable, function: Proxyowner->Handle, done: chan of int)
{
    (status, unused) := proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -2, 1000);
    done <-= status;
}


# EN: Compare owner-visible scalar against the original Cell and expected effect.
# RU: Сравнить owner-visible scalar с исходной Cell и ожидаемым effect.
read(endpoint: ref Proxyowner->Endpoint, variable: Proxyowner->Handle, original: ref Proxyhost->Cell, expected: int)
{
    empty := Proxyowner->Handle(big 0, 0, big 0);
    (status, value) := proxy->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 5000);
    if (status != 0 || value != expected || original.value != expected) {
        raise "proxy did not read the original mutable variable";
    }
}


# EN: Reopen on the same permanent owner with identical slot/generation. The old
# session handle must refuse before resolving the replacement's actual managed cell.
# RU: Переоткрыть на том же owner с одинаковыми slot/generation. Старый session
# handle обязан отказать до разрешения replacement managed cell.
reopen()
{
    native := load Proxyhost Proxyhost->PATH;
    first := ref Proxyhost->Cell(31);
    second := ref Proxyhost->Cell(50);
    if (native->open(1, 4) != 0) {
        raise "first native session";
    }
    (status, oldsession, oldslot, oldgen) := native->publish(first, Proxyhost->Variable);
    if (status != 0 || native->close() != 0 || native->open(1, 4) != 0) {
        raise "native session close/reopen";
    }
    (newstatus, session, slot, generation) := native->publish(second, Proxyhost->Variable);
    if (newstatus != 0 || session == oldsession || slot != oldslot || generation != oldgen) {
        raise "reopen fixture did not reuse slot with a new session";
    }
    if (native->drop(oldsession, oldslot, oldgen) == 0) {
        raise "old session release affected a replacement root";
    }
    (admitted, target, left, right) := native->begin(big 1, Proxyhost->Read, oldsession,
        oldslot, oldgen, big 0, 0, big 0, big 0, 0, big 0, 0, 0);
    if (admitted == 0 || target != nil) {
        raise "old session handle resolved a replacement root";
    }
    (admitted, target, left, right) = native->begin(big 1, Proxyhost->Read, session,
        slot, generation, big 0, 0, big 0, big 0, 0, big 0, 0, 0);
    if (admitted != 0 || target != second || target.value != 50 ||
        native->finish(big 1) != 0 || native->close() != 0) {
        raise "new session did not resolve its original root";
    }
}


# EN/RU: Verify callback owner identity, ref alias effects, timeout outcomes and kill cleanup.
# Проверить owner identity callback, ref aliases, timeout outcomes и kill cleanup.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    proxy = load Proxyowner Proxyowner->PATH;
    handler := load Proxyhandler "handler.dis";
    if (proxy == nil || handler == nil) {
        raise "proxy test modules unavailable";
    }
    proxy->init();
    reopen();
    ready := chan of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    original := ref Proxyhost->Cell(10);
    capture := ref Proxyhost->Cell(0);
    spawn proxy->serve(handler, original, capture, ready);
    (endpoint, variable, function) := <-ready;
    empty := Proxyowner->Handle(big 0, 0, big 0);
    read(endpoint, variable, original, 10);
    (nestedstatus, nestedvalue) := proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -4, 5000);
    if (nestedstatus != 0 || nestedvalue != 7777) {
        raise "nested proxy cycle refusal failed";
    }
    (status, value) := proxy->call(endpoint, Proxyowner->Write, variable, empty, empty, 0, 20, 5000);
    if (status != 0 || value != 20) {
        raise "proxy write failed";
    }
    done := chan of int;
    for (index := 0; index < Clients; index++) {
        spawn client(endpoint, variable, function, done);
    }
    for (index = 0; index < Clients; index++) {
        <-done;
    }
    read(endpoint, variable, original, 20 + Clients * Rounds * 2);
    spawn delayed(endpoint, variable, function, done);
    for (attempt := 0; original.value != -2222 && attempt < 10000; attempt++) {
        sys->sleep(1);
    }
    if (original.value != -2222) {
        raise "delayed callback never entered before deadline";
    }
    if (<-done != Proxyowner->Unknown) {
        raise "delivered timeout was called cancelled";
    }
    original.value = 20 + Clients * Rounds * 2;
    read(endpoint, variable, original, 22 + Clients * Rounds * 2);
    (status, value) = proxy->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -1, 5000);
    if (status != Proxyowner->OwnerError) {
        raise "callback error was not returned";
    }
    read(endpoint, variable, original, 21 + Clients * Rounds * 2);
    (status, value) = proxy->call(endpoint, Proxyowner->Invoke, variable, variable, variable, 2, 1, 5000);
    if (status == 0) {
        raise "wrong-kind function target admitted";
    }
    orphan := ref Proxyowner->Endpoint(chan of ref Proxyowner->Request);
    (status, value) = proxy->call(orphan, Proxyowner->Read, variable, empty, empty, 0, 0, 10);
    if (status != Proxyowner->NotSent) {
        raise "undelivered timeout lost admission boundary";
    }
    second := ref Proxyhost->Cell(4);
    context := ref Proxyhost->Cell(0);
    spawn proxy->serve(handler, second, context, ready);
    (other, secondvar, secondfn) := <-ready;
    (status, value) = proxy->call(other, Proxyowner->Invoke, secondfn, variable, variable, 2, 9, 5000);
    if (status == 0 || second.value != 4) {
        raise "foreign ref silently aliased a local slot";
    }
    (status, value) = proxy->call(other, Proxyowner->Drop, variable, empty, empty, 0, 0, 5000);
    if (status == 0) {
        raise "foreign release affected a local slot";
    }
    spawn blocked(endpoint, variable, function, done);
    for (attempt = 0; original.value != 777 && attempt < 1000; attempt++) {
        sys->sleep(1);
    }
    if (original.value != 777) {
        raise "blocking callback never entered";
    }
    (status, value) = proxy->call(other, Proxyowner->Invoke, secondfn, secondvar, secondvar, 2, 2, 5000);
    if (status != 0 || value != 8) {
        raise "independent owner stalled behind blocked owner";
    }
    if (control->killwait(capture.value) != Processctl->Retired || <-done != Proxyowner->Unknown) {
        raise "abandoned callback retirement failed";
    }
    (status, value) = proxy->call(other, Proxyowner->Drop, secondvar, empty, empty, 0, 0, 5000);
    if (status != 0) {
        raise "explicit proxy release failed";
    }
    (status, value) = proxy->call(other, Proxyowner->Read, secondvar, empty, empty, 0, 0, 5000);
    if (status == 0) {
        raise "released proxy remained valid";
    }
    (status, value) = proxy->call(other, Proxyowner->Stop, empty, empty, empty, 0, 0, 5000);
    if (status != 0) {
        raise "owner close failed";
    }
    sys->print("PASS guest owner proxies: variables functions refs aliases deadlines independent-owner kill cleanup\n");
}
