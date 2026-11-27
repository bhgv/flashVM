# Copyright (c) 2026 bhgv
# EN: Read, forward and retire references to one exact FlashVM module export.
# RU: Чтение, пересылка и отзыв ссылок на точный экспорт модуля FlashVM.
implement Client;
include "sys.m";
include "styxnamed.m";
include "styxmodref.m";

Client: module { init: fn(); };
sys: Sys; # Paired runtime operations.
named: Styxnamed; # Existing heterogeneous channel API.
refs: Styxmodref; # Typed proxy to the exact paired publication.
Cell: import refs;
Port: import named;
Export: con "mounted/ref-pair.dis/.var/counter"; # Reused by the next publication.
Marker: con "/module-ref-ready"; # Wait until both captures are complete.

include "named_movie_upload.b";


# EN: Wait for a visible export with an identity different from previous.
# RU: Дождаться видимого экспорта с идентичностью, отличной от previous.
publication(previous: big): big
{
    for (i := 0; i < 200; ++i) {
        (ok, meta) := sys->stat("/peer/" + Export);
        if (ok >= 0 && meta.length == big 4 && meta.qid.path != previous) {
            return meta.qid.path;
        }
        sys->sleep(10);
    }
    raise sys->sprint("FlashVM export did not publish: %r");
}


# EN: Compare two counted descriptors exactly, including their scopes.
# RU: Побайтово сравнить два дескриптора, включая области.
same(a, b: array of byte): int
{
    if (len a != len b) {
        return 0;
    }
    for (i := 0; i < len a; ++i) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}


# EN: Return one for a frame rejected by the paired module-reference adapter.
# RU: Вернуть единицу, если адаптер пары отказал кадру ссылки.
refused(frame: array of byte): int
{
    {
        cell := refs->adopt(frame);
        cell.close();
    } exception e {
    "*" =>
        return 1;
    }
    return 0;
}


# EN: Return whether a held or deferred alias still admits reads.
# RU: Проверить, допускает ли чтение открытая или отложенная ссылка.
readable(cell: ref Cell): int
{
    {
        current := cell.read();
    } exception e {
    "*" =>
        return 0;
    }
    return 1;
}


# EN: Return whether the adapter allowed a write through this reference.
# RU: Проверить, разрешил ли адаптер запись по этой ссылке.
canwrite(cell: ref Cell): int
{
    {
        cell.write(19);
    } exception e {
    "*" =>
        return 0;
    }
    return 1;
}


# EN: Restart the same movie so the module name is reused by a new generation.
# RU: Перезапустить фильм, чтобы имя модуля досталось новому поколению.
restart()
{
    if (sys->remove(Marker) < 0) {
        raise sys->sprint("cannot clear module reference readiness: %r");
    }
    state := sys->open("/peer/.state", Sys->OWRITE);
    bytes := array of byte "restart";
    if (state == nil || sys->write(state, bytes, len bytes) != len bytes) {
        raise sys->sprint("cannot restart the module provider: %r");
    }
}


# EN: Drive both reference directions and check stale identities after channel close.
# RU: Передать ссылки в обе стороны и проверить отзыв после закрытия канала.
init()
{
    sys = load Sys Sys->PATH;
    named = load Styxnamed Styxnamed->PATH;
    refs = load Styxmodref Styxmodref->PATH;
    if (named == nil || refs == nil) {
        raise "cannot load named module-reference adapters";
    }
    named->init();
    refs->init();
    pipe := named->open("/module-refs");
    start("modref.swf");
    first := publication(big 0);
    if (sys->chdir("/peer") < 0) {
        raise "cannot change the guest cwd before capture";
    }
    cell := refs->capture(Export, 1);
    observer := refs->capture(Export, 0);
    wire := cell.payload();
    reading := observer.payload();
    foreign := array[len wire] of byte;
    foreign[0:] = wire;
    foreign[13] ^= byte 1;
    if (cell.read() != 41 || observer.read() != 41 || !refused(foreign) || canwrite(observer)) {
        raise "mode or scope of the imported export differs";
    }
    foreign[0:] = wire;
    foreign[4] ^= byte 4;
    if (!refused(foreign)) {
        raise "an unsupported descriptor flag was accepted";
    }
    ready := sys->create(Marker, Sys->OWRITE, 8r666);
    if (ready == nil) {
        raise sys->sprint("cannot signal module reference readiness: %r");
    }
    ready = nil;
    pipe.send(wire);
    pipe.send(reading);
    proof := pipe.recv();
    if (int proof[0] != Styxnamed->Text || string proof[1:] != "41,-73,-73") {
        raise sys->sprint("FlashVM alias update or service visibility differs: %s", string proof[1:]);
    }
    returned := pipe.recv();
    observed := pipe.recv();
    if (!same(wire, returned) || !same(reading, observed)) {
        raise "the owner changed the identity or permissions while forwarding";
    }
    alias := refs->adopt(returned);
    unopened := refs->adopt(observed);
    pipe.close();
    actual := alias.read();
    if (actual != -73) {
        raise sys->sprint("returned alias read %d, expected -73", actual);
    }
    minimum := -2147483647 - 1;
    alias.write(minimum);
    if (cell.read() != minimum || observer.read() != minimum) {
        raise "the negative Int32 boundary did not reach the original export";
    }
    alias.write(2147483647);
    if (cell.read() != 2147483647 || observer.read() != 2147483647) {
        raise "writing through the returned alias missed the original export";
    }
    restart();
    publication(first);
    if (readable(cell) || readable(alias) || readable(unopened)) {
        raise "old module reference was rebound after restart";
    }
    fresh := refs->capture(Export, 0);
    if (fresh.read() != 41 || canwrite(fresh)) {
        raise "replacement value or read-only mode differs";
    }
    fresh.close();
    unopened.close();
    alias.close();
    observer.close();
    cell.close();
    sys->print("PASS native modref RPC paired VMs\n");
}
