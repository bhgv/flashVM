# Copyright (c) 2026 bhgv
# EN: Real guest layer enumeration, indexed bind/move, per-file override and namespace isolation.
# RU: Guest layer enumeration, indexed bind/move, per-file override и namespace isolation.
implement Check;
include "sys.m";
include "vfsctl.m";
Check: module { init: fn(); };
sys: Sys; # Builtin reference initialized before guest operations.
ctl: Vfsctl; # Typed pure Limbo layer API.


# EN/RU: Raise a label on a false observed contract.
# Вызвать raise с label при нарушении observed contract.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Read one byte through the ordinary namespace, not a native test-only route.
# Прочитать byte через ordinary namespace; no native test-only bypass.
value(path: string): string
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "ordinary file read: " + path);
    return string bytes;
}


# EN/RU: Inspect all effective providers and validate copied configuration/properties.
# Проверить всех providers и copied configuration/properties; return snapshot/list.
view(path: string, count: int): (big,list of ref Vfsctl->Layer)
{
    (status, revision, layers) := ctl->layers(path);
    check(status == Vfsctl->Ready && len layers == count, "layer count: " + path);
    i := 0;
    for (entries := layers; entries != nil; entries = tl entries) {
        card := hd entries;
        check(card.position == i && card.count == count && card.metadata == 0 &&
            card.kind == Vfsctl->Native && card.mount != nil && card.relative != nil,
            "layer properties: " + path);
        check((card.access & (Vfsctl->Read << 8)) != 0 && (card.access & Vfsctl->Read) != 0,
            "effective access observation");
        check(card.session == big 0 && card.generation == big 0, "native route invented provider session");
        i++;
    }
    return (revision, layers);
}


# EN/RU: A copied namespace can reorder its own stack without changing the parent.
# Copied namespace переставляет собственный stack, не меняя parent group.
child(done: chan of int)
{
    check(sys->pctl(Sys->FORKNS, nil) > 0, "fork namespace");
    (revision, layers) := view("view/f", 3);
    check(ctl->move("view/f", revision, (hd layers).id, -1) == 0, "child move");
    check(value("view/f") == "B", "child order");
    done <-= 1;
}


# EN/RU: Perform actual Sys/native bridge operations and verify resource lifetime.
# Выполнить real Sys/native bridge operations; проверить resource lifetime.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Vfsctl Vfsctl->PATH;
    check(ctl != nil, "load Vfsctl");
    ctl->init();
    check(sys->bind("a", "view", Sys->MREPL) == 0, "classic replace");
    check(sys->bind("b", "view", Sys->MAFTER) == 0, "classic bottom");
    check(ctl->bind("c", "view", Vfsctl->MINS, 1) == 0, "middle directory insertion");
    (revision, layers) := view("view", 3);
    check(value("view/f") == "A" && value("view/g") == "a", "initial traversal");
    middle := hd tl layers;
    first := hd layers;
    check(ctl->move("view", revision, middle.id, 0) == 0, "directory move to top");
    check(value("view/f") == "C" && value("view/g") == "c", "directory order after move");
    check(ctl->move("view", revision, first.id, -1) == Vfsctl->Stale, "stale edit accepted");
    (revision, layers) = view("view/f", 3);
    inherited := hd tl layers;
    old := sys->open("view/f", Sys->OREAD);
    check(old != nil && ctl->move("view/f", revision, inherited.id, 0) == 0, "per-file inherited move");
    check(value("view/f") == "A" && value("view/g") == "c", "per-file move changed sibling");
    (nil, parents) := view("view", 3);
    check((hd parents).id == middle.id, "per-file move changed directory binding");
    bytes := array[1] of byte;
    check(sys->read(old, bytes, 1) == 1 && string bytes == "C", "move retargeted open FD");
    (revision, layers) = view("view/f", 3);
    mountpath := (hd layers).mount;
    check(len mountpath >= 7 && mountpath[len mountpath - 7:] == "/view/f",
        "file override has no exact virtual mount point");
    id := (hd layers).id;
    check(ctl->move("view/f", revision, id, -1) == 0, "file move to bottom");
    check(value("view/f") == "C" && value("view/g") == "c", "file bottom order");
    (revision, layers) = view("view/f", 3);
    check((hd tl tl layers).id == id, "exact file move changed ID");
    check(ctl->move("view/f", revision, id, 3) == Vfsctl->Error &&
        ctl->bind("b/f", "view/f", Vfsctl->MINS, 99) == Vfsctl->Error &&
        ctl->bind("b/f", "view/f", Vfsctl->MINS|Vfsctl->MBEFORE, 0) == Vfsctl->Error,
        "invalid edit accepted");
    (same, nil) := view("view/f", 3);
    check(same == revision, "invalid edit changed namespace");
    done := chan[1] of int;
    spawn child(done);
    check(<-done == 1 && value("view/f") == "C", "copied edit changed parent");
    check(ctl->bind("b/f", "view/f", Vfsctl->MINS, -1) == 0, "file bottom insert");
    (nil, layers) = view("view/f", 4);
    check((hd tl tl tl layers).position == 3, "negative insertion position");
    (status, nil, provider) := ctl->layer("/provider/f", big -1, 0);
    check(status == Vfsctl->Ready && provider.kind == Vfsctl->Provider &&
        provider.session == big 1 && provider.generation == big 1 &&
        provider.capabilities == Vfsctl->Stablebind && provider.relative == "f" &&
        (provider.access & 7) == Vfsctl->Read, "provider properties/session/rights");
    check(ctl->bind("/provider/f", "view/f", Vfsctl->MINS|Vfsctl->MAUTORWX, 0) == 0 && value("view/f") == "P",
        "provider file top insertion");
    writable := sys->open("view/f", Sys->OWRITE);
    check(writable != nil && sys->write(writable, array of byte "C", 1) == 1 && value("view/f") == "P",
        "MAUTORWX did not keep upper read/lower write selection");
    providers: list of ref Vfsctl->Layer;
    (status, revision, providers) = ctl->layers("view/f");
    check(status == Vfsctl->Ready && len providers == 5 && (hd providers).kind == Vfsctl->Provider,
        "mixed native/provider stack");
    check(ctl->move("view/f", revision, (hd providers).id, -1) == 0 && value("view/f") == "C",
        "provider move below native branches");
    check(sys->unmount(nil, "view/f") == 0 && value("view/f") == "C" && value("view/g") == "c",
        "unmount did not restore parent inheritance");
    check(sys->unmount(nil, "/provider") == 0, "provider root unmount");
    check(sys->bind("a", "view", Vfsctl->MINS) == -1, "standard Sys.bind consumed extension index bits");
    sys->print("PASS Vfsctl all layers/properties/MINS/move/stale/file projection/sibling/open FD/FORKNS/unmount\n");
}
