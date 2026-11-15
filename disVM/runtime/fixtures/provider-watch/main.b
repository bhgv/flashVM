# Copyright (c) 2026 bhgv
# EN: Event files through Sys/Providerwatch, capability lifetime and single-record reads.
# RU: Event files через Sys/Providerwatch, capability lifetime и single-record reads.
implement WatchCheck;
include "sys.m";
include "providerwatch.m";
WatchCheck: module { init: fn(); };
sys: Sys; # Immutable builtin after initialization.
watcher: Providerwatch; # Pure Limbo client, shared only after init.


# EN/RU: Raise a diagnostic on a false observed guest contract.
# Вызвать raise diagnostic при false guest contract.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Consume ordinary per-thread system records without any watch registration.
# Return VFS class union; AGAIN indicates an empty live queue. No guest ack is sent.
# Читать per-thread records без registration; вернуть VFS classes, без guest ack.
sysevents(fd: ref Sys->FD): int
{
    bytes := array[9000] of byte;
    observed := 0;
    for (i := 0; i < 64; i++) {
        n := sys->readn(fd, bytes, len bytes);
        if (n < 0 && sys->sprint("%r") == "native input temporarily unavailable") {
            return observed;
        }
        check(n > 0, "system mailbox returned error/EOF while resident alive");
        (count, words) := sys->tokenize(string bytes[:n], " \n");
        check(count == 8 && hd words == "system", "system mailbox frame");
        words = tl words;
        kind := int hd words;
        words = tl words;
        if (kind == 1 || kind == 3) {
            observed |= int hd words;
        }
    }
    raise "system mailbox did not become idle";
    return 0;
}


# EN/RU: Read one current source byte through ordinary Sys, no hidden native path.
# Прочитать source byte через Sys, без скрытого native path.
value(): int
{
    fd := sys->open("view/data", Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "source state reread");
    return int bytes[0];
}


# EN/RU: Mutate only private fixture controls to publish/deny/reconnect the source.
# Менять только private fixture controls для publish/deny/reconnect source.
command(fd: ref Sys->FD, text: string)
{
    bytes := array of byte text;
    check(sys->write(fd, bytes, len bytes) == len bytes, "fixture command: " + text);
}


# EN/RU: Require a complete helper event; caller explicitly rereads then acks.
# Проверить complete helper event; caller сам делает reread и ack.
event(watch: ref Providerwatch->Watch): ref Providerwatch->Event
{
    (status, result) := watcher->next(watch);
    check(status == Providerwatch->EventReady && result != nil, "expected watch event");
    return result;
}


# EN/RU: A fresh namespace cannot reopen a hidden endpoint; the inherited FD
# remains a capability. No new global provider ID or registry is available.
# NEWNS не открывает hidden endpoint; inherited FD остаётся capability.
isolated(watch: ref Providerwatch->Watch, done: chan of int)
{
    check(sys->pctl(Sys->FORKNS, nil) > 0 && sys->chdir("jail") == 0 &&
        sys->pctl(Sys->NEWNS, nil) > 0, "isolated namespace");
    check(sys->open("/dev/sysevents", Sys->OREAD) == nil, "NEWNS bypassed namespace for system device");
    check(sys->open("/watch/events", Sys->ORDWR) == nil, "hidden watch reopened outside namespace");
    (status, nil) := watcher->next(watch);
    check(status == Providerwatch->Idle, "inherited observer invalidated by NEWNS");
    done <-= 1;
}


# EN/RU: Exercise ORDWR shared native offsets, explicit observer acknowledgements,
# masks, races, source policy revocation, stale epochs, single-record reads,
# namespace hiding and unmounted-but-open descriptor capabilities.
# Проверить ORDWR, observer ack/masks/races/policy/epochs/records и namespace lifetime.
init()
{
    sys = load Sys Sys->PATH;
    system := sys->open("/dev/sysevents", Sys->OREAD);
    check(system != nil && sysevents(system) != 0, "automatic system events required Limbo registration");
    check(sys->write(system, array of byte "ack\n", 4) == -1, "system events accepted guest acknowledgement");
    watcher = load Providerwatch Providerwatch->PATH;
    check(watcher != nil, "load Providerwatch");
    watcher->init();
    native := sys->open("native", Sys->ORDWR);
    data := array[5] of byte;
    check(native != nil && sys->read(native, data, 2) == 2 && string data[:2] == "AB", "ORDWR initial read");
    check(sys->write(native, array of byte "X", 1) == 1 && sys->read(native, data, 2) == 2 &&
        string data[:2] == "DE", "ORDWR shared offset");
    check(sys->seek(native, big 0, Sys->SEEKSTART) == big 0 && sys->readn(native, data, 5) == 5 &&
        string data == "ABXDE", "ORDWR seek/nonappend/nontruncate");
    check(sys->open("missing", Sys->ORDWR) == nil, "ORDWR created absent file");
    (bad, ignored) := watcher->open("ordinary", Providerwatch->All);
    check(bad == Providerwatch->Error && ignored == nil, "ordinary file accepted as event protocol");
    ctl := sys->open("view/ctl", Sys->OWRITE);
    check(ctl != nil, "source fixture capability");
    (status, first) := watcher->open("watch/events", Providerwatch->All);
    check(status == 0, "first watch open");
    check(sys->bind("watch/events", "alias", Sys->MREPL) == 0, "event file bind alias");
    second: ref Providerwatch->Watch;
    (status, second) = watcher->open("alias", Providerwatch->All);
    check(status == 0, "second alias observer");
    firstevent := event(first);
    check(value() == int 'A' && watcher->ack(first, firstevent) == 0, "initial reread/ack");
    (status, nil) = watcher->next(first);
    check(status == Providerwatch->Idle, "ack did not drain first");
    secondevent := event(second);
    check(secondevent.kinds == Providerwatch->All && value() == int 'A' &&
        watcher->ack(second, secondevent) == 0, "independent observer globally cleared");
    sysevents(system);
    small := sys->open("/dev/sysevents", Sys->OREAD);
    check(small != nil, "second owner inbox FD");
    command(ctl, "change\n");
    shortbytes := array[5] of byte;
    check(sys->read(small, shortbytes, len shortbytes) == -1, "short system event was split");
    small = nil;
    check(sysevents(system) == 5, "VFS change was not automatically delivered to owner mailbox");
    firstevent = event(first);
    check(firstevent.kinds == (Providerwatch->Rights|Providerwatch->Content) && value() == int 'B', "changed state reread");
    command(ctl, "race\n");
    check(watcher->ack(first, firstevent) == 0, "captured ack");
    firstevent = event(first);
    check(firstevent.kinds == Providerwatch->Content && value() == int 'C', "ack swallowed racing event");
    command(ctl, "reconnect\n");
    check(watcher->ack(first, firstevent) == Providerwatch->Error, "stale epoch ack accepted");
    firstevent = event(first);
    check(firstevent.session != secondevent.session && value() == int 'D' &&
        watcher->ack(first, firstevent) == 0, "new epoch full refresh");
    command(ctl, "deny\n");
    (status, nil) = watcher->next(first);
    check(status == Providerwatch->Error, "revoked existing watch readable");
    command(ctl, "allow\n");
    firstevent = event(first);
    check(value() == int 'D' && watcher->ack(first, firstevent) == 0, "restored watch state");
    check(watcher->mask(first, Providerwatch->Rights) == 0, "select rights");
    command(ctl, "load\n");
    (status, nil) = watcher->next(first);
    check(status == Providerwatch->Idle, "mask ignored");
    check(watcher->mask(first, Providerwatch->All) == 0, "restore all categories");
    firstevent = event(first);
    check(firstevent.kinds == (Providerwatch->Load|Providerwatch->Network) &&
        watcher->ack(first, firstevent) == 0, "filter change acknowledged unseen categories");
    done := chan[1] of int;
    spawn isolated(first, done);
    check(<-done == 1, "isolated observer completion");
    check(sys->unmount(nil, "watch") == 0 && sys->unmount(nil, "alias") == 0 &&
        sys->unmount(nil, "view") == 0, "projection/source unmount");
    check(sys->open("watch/events", Sys->ORDWR) == nil, "unmounted projection reopened");
    command(ctl, "change\n");
    firstevent = event(first);
    check(watcher->ack(first, firstevent) == 0, "unmount destroyed retained observer");
    sys->print("PASS provider watch guest ORDWR/handshake/ack/race/mask/revoke/epoch/records/namespace/lifetime\n");
}
