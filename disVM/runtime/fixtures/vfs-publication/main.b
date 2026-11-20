# Copyright (c) 2026 bhgv
# EN: Real cross-thread publication, prepared trees, automatic notifications and capability rejection.
# RU: Cross-thread publication, prepared trees, automatic notifications и capability rejection.
implement Check;
include "sys.m";
include "processctl.m";
include "vfspublish.m";
Check: module { init: fn(); };
sys: Sys; # Shared only after loading.
api: Vfsexport; # Exact native capability API.
pub: Vfspublish; # One-request copied reader.
pc: Processctl; # Actual permanent guest PID oracle.


# EN/RU: Fail an observable guest contract explicitly.
# Явно завершить проверку observable guest contract.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Read through ordinary namespace and FD dispatch.
# Читать через ordinary namespace и FD dispatch.
value(path: string): string
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "ordinary read " + path);
    return string bytes;
}


# EN/RU: A native-captured exact file can be shared after sender retirement.
# Native-captured exact file остаётся жив после sender retirement.
sender(out: chan of ref Vfsexport->Root)
{
    out <-= api->capture("data/b", 0, Vfsexport->Read);
}


# EN/RU: Submit from another actual OS owner; never fabricate peer PID in Limbo.
# Отправить с другого OS owner; peer PID не задаётся из Limbo.
requester(endpoint: ref Vfsexport->Endpoint, out: chan of (int,ref Vfsexport->Ticket))
{
    out <-= (pc->pid(), api->submit(endpoint, "requested-λ", 2, 5));
}


# EN/RU: Competing aliases may transfer exactly one granted tree.
# Competing aliases могут передать ровно одно granted tree.
taker(ticket: ref Vfsexport->Ticket, out: chan of ref Vfsexport->Root)
{
    out <-= api->take(ticket);
}


# EN/RU: A child may mount a shared capability only in its own copied namespace.
# Child монтирует shared capability в своём copied namespace.
isolated(root: ref Vfsexport->Root, done: chan of int)
{
    check(sys->pctl(Sys->FORKNS, nil) > 0, "copy namespace");
    check(api->mount(root, "file", 0, 0) == 0 && value("file") == "A", "child capability mount");
    done <-= 1;
}


# EN/RU: Drain complete existing VFS records, never ask next/status to generate notifications.
# Читать existing VFS records; next/status не генерируют уведомления.
notices(fd: ref Sys->FD): int
{
    bytes := array[9000] of byte;
    observed := 0;
    for (i := 0; i < 64; i++) {
        n := sys->read(fd, bytes, len bytes);
        if (n < 0 && sys->sprint("%r") == "native input temporarily unavailable") {
            return observed;
        }
        check(n > 0, "live events unexpectedly ended");
        (count, words) := sys->tokenize(string bytes[:n], " \n");
        check(count == 8 && hd words == "system", "one complete system record");
        words = tl words;
        kind := int hd words;
        words = tl words;
        if (kind == 1 || kind == 3) {
            observed |= int hd words;
        }
    }
    raise "event queue did not drain";
    return 0;
}


# EN/RU: Forged records have the same visible prefix but never native authority.
# Forged records имеют тот же public prefix, но не native authority.
forgery()
{
    caught := 0;
    {
        api->mount(ref Vfsexport->Root(0), "out", 0, 0);
    } exception {
        "*" => caught = 1;
    }
    check(caught, "forged Root accepted");
    caught = 0;
    {
        api->endpoint(ref Vfsexport->Owner(0));
    } exception {
        "*" => caught = 1;
    }
    check(caught, "forged Owner accepted");
    caught = 0;
    {
        api->submit(ref Vfsexport->Endpoint(0), "data", 2, 4);
    } exception {
        "*" => caught = 1;
    }
    check(caught, "forged Endpoint accepted");
    caught = 0;
    {
        api->status(ref Vfsexport->Ticket(0));
    } exception {
        "*" => caught = 1;
    }
    check(caught, "forged Ticket accepted");
    caught = 0;
    {
        api->describe(ref Vfsexport->Request(0));
    } exception {
        "*" => caught = 1;
    }
    check(caught, "forged Request accepted");
}


# EN/RU: Optional private refusal is caught before normal capture, then constructor recovers.
# Private refusal перехватывается до normal capture; после него constructor работает.
construction()
{
    root: ref Vfsexport->Root;
    caught := 0;
    {
        root = api->capture("data", 0, 5);
    } exception {
        "VFS capability publication refusal" => caught = 1;
    }
    if (caught) {
        check(root == nil, "refused publication changed result");
        root = api->capture("data", 0, 5);
        check(root != nil, "capture did not recover after refusal");
        sys->print("PASS VFS capability constructor refusal\n");
    }
}


# EN/RU: Change a shared exact capability from another Limbo process.
# Изменить общую точную ссылку из другого процесса Limbo.
setvariable(cell: ref Vfsexport->Root, done: chan of int)
{
    done <-= api->intset(cell, -2);
}


# EN/RU: Verify live scalar sharing between typed calls and the published file.
# Проверить общий скаляр между типизированными вызовами и опубликованным файлом.
variables()
{
    cell := api->intvar(41);
    check(cell != nil, "create integer variable");
    (status, initial) := api->intget(cell);
    check(status == 1 && initial == 41, "initial integer variable");
    tree := api->overlay(nil, cell, "/.var/count", Vfsexport->Read | Vfsexport->Write, 0);
    readonly := api->overlay(nil, cell, "/.var/count", Vfsexport->Read, 0);
    check(tree != nil && readonly != nil, "compose integer publication");
    read := api->open(readonly, ".var/count", Sys->OREAD);
    file := api->open(tree, ".var/count", Sys->ORDWR);
    check(read != nil && file != nil && api->open(readonly, ".var/count", Sys->OWRITE) == nil &&
        !api->intset(tree, 999), "integer projection rights");
    done := chan of int;
    spawn setvariable(cell, done);
    check(<-done == 1, "child sets integer variable");
    bytes := array[4] of byte;
    check(sys->read(read, bytes, 4) == 4 && int bytes[0] == 255 && int bytes[3] == 254,
        "opened file observes typed update");
    bytes = array[] of {byte 0, byte 1, byte 0, byte 3};
    check(sys->write(file, bytes, 3) == -1, "partial integer write accepted");
    (status, initial) = api->intget(cell);
    check(status == 1 && initial == -2, "partial integer write changed value");
    check(sys->write(file, bytes, 4) == 4 && sys->seek(read, big 0, Sys->SEEKSTART) == big 0 &&
        sys->read(read, bytes, 4) == 4 && int bytes[1] == 1 && int bytes[3] == 3,
        "file aliases share update");
    (status, initial) = api->intget(cell);
    check(status == 1 && initial == 65539, "typed read observes file update");
    sys->print("PASS Limbo integer variable publication\n");
}


# EN/RU: Complete actual API handoff; the grant is a different ordinary prepared tree.
# Полная передача через API; grant — другое ordinary prepared tree.
init()
{
    sys = load Sys Sys->PATH;
    api = load Vfsexport Vfsexport->PATH;
    pub = load Vfspublish Vfspublish->PATH;
    pc = load Processctl Processctl->PATH;
    check(sys != nil && api != nil && pub != nil && pc != nil, "load publication API");
    pub->init();
    construction();
    variables();
    forgery();
    check(api->capture("data", 99, 5) == nil && api->capture("data", 0, 0) == nil, "invalid capture accepted");
    base := api->capture("data", 0, 5);
    check(base != nil, "directory capture");
    roots := chan of ref Vfsexport->Root;
    spawn sender(roots);
    source := <-roots;
    check(source != nil, "shared root after sender exit");
    tree := api->overlay(base, nil, "/secret", 0, Vfsexport->Tree);
    tree = api->overlay(tree, source, "/a", 4, 0);
    tree = api->overlay(tree, source, "/service/input", 4, 0);
    check(tree != nil && api->overlay(tree, source, "/../escape", 4, 0) == nil, "tree composition");
    tree.reserved = 12345;
    owner := api->announce(2, Vfsexport->Prepare, nil);
    other := api->announce(2, Vfsexport->Prepare, nil);
    endpoint := api->endpoint(owner);
    check(owner != nil && other != nil && endpoint != nil, "publication creation");
    check(api->mount(api->events(owner), "notice", 0, 0) == 0, "notification source bind");
    events := sys->open("/dev/sysevents", Sys->OREAD);
    check(events != nil, "system event source");
    notices(events);
    done := chan of (int,ref Vfsexport->Ticket);
    spawn requester(endpoint, done);
    (pid, ticket) := <-done;
    check(pid != pc->pid() && ticket != nil, "cross-thread request submission");
    check((notices(events) & 4) != 0, "request notification required next or polling");
    check(api->status(ticket) == Vfsexport->Pending, "request did not remain pending");
    request := pub->next(owner);
    check(request != nil && request.pid == pid && request.kind == 2 && request.rights == 5 &&
          request.name == "requested-λ" && pub->next(owner) == nil, "copied one-request fields");
    check(!pub->reply(other, request, tree), "wrong publication answered request");
    check(pub->reply(owner, request, tree) && !pub->reply(owner, request, tree), "explicit one-use reply");
    check(api->status(ticket) == Vfsexport->Granted && api->status(ticket) == Vfsexport->Granted,
          "status consumed grant");
    grant := api->take(ticket);
    check(grant != nil && api->take(ticket) == nil && api->status(ticket) == Vfsexport->Consumed, "one-use take");
    (status, mode, directory) := api->info(grant);
    check(status == 0 && directory == 1 && (mode & 8r222) == 0, "granted root inspection before mount");
    (status, nil, directory) = api->info(source);
    check(status == 0 && directory == 0, "captured exact file inspection");
    check(api->mount(grant, "out", 0, 0) == 0 && value("out/a") == "B" &&
          value("out/service/input") == "B", "chosen tree not ordinary mounted resource");
    check(sys->open("out/secret", Sys->OREAD) == nil && sys->open("out/link", Sys->OREAD) == nil &&
          sys->open("out/a", Sys->OWRITE) == nil, "hidden, symlink or rights-ceiling bypass");
    pending := api->submit(endpoint, "cancel", 1, 4);
    cancelled := pub->next(owner);
    check(pending != nil && cancelled != nil && api->cancel(pending) && !api->cancel(pending) &&
          !pub->reply(owner, cancelled, source), "cancelled request resurrected");
    check(api->cancel(ticket), "release consumed ticket quota");
    first := api->submit(endpoint, "one", 1, 4);
    second := api->submit(endpoint, "two", 1, 4);
    check(first != nil && second != nil && api->submit(endpoint, "overflow", 1, 4) == nil, "queue limit");
    check(api->close(owner) && !api->close(owner) && api->status(first) == Vfsexport->Denied &&
          api->status(second) == Vfsexport->Denied && api->submit(endpoint, "closed", 1, 4) == nil, "close pending/admission");
    check(value("out/a") == "B", "accepted grant did not survive publisher close");
    api->cancel(first);
    api->cancel(second);
    api->cancel(ticket);
    api->close(other);
    direct := api->announce(1, 0, source);
    directticket := api->submit(api->endpoint(direct), "different-name", 1, 4);
    check(directticket != nil && api->status(directticket) == Vfsexport->Granted &&
          pub->next(direct) == nil, "direct path incorrectly required preparation");
    check(api->mount(api->take(directticket), "file", 0, 0) == 0 && value("file") == "B", "direct exact-file mount");
    api->cancel(directticket);
    raced := api->submit(api->endpoint(direct), "race", 1, 4);
    winners := chan of ref Vfsexport->Root;
    spawn taker(raced, winners);
    spawn taker(raced, winners);
    left := <-winners;
    right := <-winners;
    check((left != nil) != (right != nil) && api->status(raced) == Vfsexport->Consumed,
          "concurrent aliases took grant twice");
    api->cancel(raced);
    copied := chan of int;
    spawn isolated(api->capture("data/a", 0, 4), copied);
    check(<-copied == 1 && value("file") == "B", "child mount changed parent namespace");
    api->close(direct);
    sys->print("PASS guest publication capabilities trees requests rights notices cancel close direct\n");
}
