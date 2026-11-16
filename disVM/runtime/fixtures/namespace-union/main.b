# Copyright (c) 2026 bhgv
# EN: Native directory layers, permission-preserving lookup and retained cursors.
# RU: Native directory layers, lookup с сохранением прав и retained cursors.
implement UnionCheck;
include "sys.m";
UnionCheck: module { init: fn(); };
sys: Sys; # Immutable builtin module before resident spawn.


# EN/RU: Require condition or raise the diagnostic label.
# Проверить condition либо raise диагностическую подпись.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Open one byte using ordinary Sys; local FD retires with this frame.
# Прочитать byte через Sys; local FD освобождается с frame.
read(path: string): int
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "union read: " + path);
    return int bytes[0];
}


# EN/RU: Optionally detach namespace before ready; read after parent's mutation.
# Отделить namespace до ready при detach; читать после изменения у parent.
reader(detach: int, ready: chan of int, command: chan of int, reply: chan of int)
{
    if (detach) {
        check(sys->pctl(Sys->FORKNS, nil) > 0, "union FORKNS");
    }
    ready <-= 1;
    <-command;
    reply <-= read("view/token");
}


# EN/RU: Enumerate a retained union FD, after first layer removal, including two token names.
# Прочитать retained union FD, после удаления первого слоя, включая два имени token.
listing(fd: ref Sys->FD)
{
    tokens := 0;
    a := 0;
    b := 0;
    c := 0;
    for (;;) {
        (count, entries) := sys->dirread(fd);
        check(count >= 0, "union directory read");
        if (count == 0) {
            break;
        }
        for (i := 0; i < count; i++) {
            case entries[i].name {
            "token" => tokens++;
            "a-only" => a++;
            "b-only" => b++;
            "c-only" => c++;
            }
        }
    }
    check(tokens == 2 && a == 0 && b == 1 && c == 1, "union directory follows current mount head");
}


# EN/RU: Build A/B/C union, create only in C, mutate host mode via stdin barrier,
# then remove individual layers/all layers and verify shared/copied/retained views.
# Собрать A/B/C union, создать только в C, изменить host mode через stdin barrier,
# затем проверить снятие слоёв и shared/copied/retained views.
init()
{
    sys = load Sys Sys->PATH;
    check(sys->bind("a", "view", Sys->MREPL) == 0, "union replace");
    check(sys->bind("b", "view", Sys->MAFTER) == 0, "union after");
    check(sys->bind("c", "view", Sys->MAFTER|Sys->MCREATE) == 0, "union create branch");
    check(sys->chdir("view") == -1, "union cwd silently dropped layers");
    check(read("view/token") == int 'A' && read("view/b-only") == int 'B' && read("view/c-only") == int 'C', "union lookup order");
    check(sys->open("view/token", Sys->OWRITE) == nil, "write fell through readonly A");
    check(sys->create("view/token", Sys->OWRITE, 8r666) == nil, "create existing fell through A");
    created := sys->create("view/new", Sys->OWRITE, 8r666);
    check(created != nil && sys->write(created, array[] of {byte 'N'}, 1) == 1, "MCREATE selection");
    check(sys->bind("view", "mirror", Sys->MREPL) == 0, "copy union bind");
    check(read("mirror/b-only") == int 'B', "copied union lower layer lost");
    check(sys->create("mirror/no-create", Sys->OWRITE, 8r666) == nil, "missing MCREATE ignored");
    held := sys->open("view/token", Sys->OREAD);
    directory := sys->open("view", Sys->OREAD);
    check(held != nil && directory != nil, "retained union opens");
    ready := chan of int;
    copycmd := chan of int;
    sharedcmd := chan of int;
    reply := chan of int;
    spawn reader(1, ready, copycmd, reply);
    spawn reader(0, ready, sharedcmd, reply);
    <-ready;
    <-ready;
    sys->print("UNION_CHMOD_READY\n");
    bytebuf := array[1] of byte;
    check(sys->read(sys->fildes(0), bytebuf, 1) == 1, "chmod release");
    output := sys->open("view/token", Sys->OWRITE);
    check(output != nil && sys->write(output, array[] of {byte 'T'}, 1) == 1, "dynamic permission refresh");
    check(sys->unmount("a", "view") == 0 && read("view/token") == int 'B', "remove first union layer");
    check(sys->read(held, bytebuf, 1) == 1 && int bytebuf[0] == int 'T', "open FD redirected after unmount");
    listing(directory);
    check(sys->seek(directory, big 0, Sys->SEEKSTART) == big 0, "union directory rewind");
    listing(directory);
    copycmd <-= 1;
    copied := <-reply;
    check(copied == int 'T', "copied namespace changed: " + string copied);
    sharedcmd <-= 1;
    check(<-reply == int 'B', "shared namespace stale");
    check(read("mirror/token") == int 'T', "source unmount redirected copied union");
    check(sys->bind("a", "view", Sys->MBEFORE) == 0 && read("view/token") == int 'T', "union before");
    check(sys->unmount("a", "view") == 0, "remove inserted layer");
    check(sys->unmount("b", "view") == 0 && read("view/token") == int 'C', "remove second union layer");
    check(sys->unmount(nil, "view") == 0 && read("view/token") == int 'V', "remove all union layers");
    check(sys->bind("c", "view", Sys->MREPL) == 0, "new mount head");
    check(sys->bind("view", "fresh", Sys->MREPL|Sys->MCREATE) == -1, "noncreatable mounted source accepted");
    check(sys->seek(directory, big 0, Sys->SEEKSTART) == big 0, "old directory rewind");
    (oldcount, oldentries) := sys->dirread(directory);
    check(oldcount == 0 && oldentries == nil, "old FD attached to new mount head");
    check(sys->unmount(nil, "view") == 0, "new mount cleanup");
    check(sys->bind("mirror", "fresh", Sys->MREPL|Sys->MCREATE) == -1, "creatable union-source copy accepted");
    check(sys->bind("a", "fresh", Sys->MBEFORE) == 0, "initial union before");
    check(read("fresh/token") == int 'T' && read("fresh/f-only") == int 'F', "original target baseline lost");
    check(sys->bind("c", "fresh", Sys->MAFTER) == 0 && read("fresh/c-only") == int 'C', "initial union fallback");
    check(sys->unmount("a", "fresh") == 0 && read("fresh/token") == int 'F', "baseline promotion");
    check(sys->unmount(nil, "fresh") == 0, "fresh stack cleanup");
    check(sys->bind("c", "after", Sys->MAFTER|Sys->MCREATE) == 0 && read("after/token") == int 'H', "initial after order");
    check(read("after/c-only") == int 'C', "initial after lower lookup");
    check(sys->unmount("c", "after") == 0 && read("after/token") == int 'H', "remove lower branch");
    check(sys->create("after/blocked", Sys->OWRITE, 8r666) == nil, "baseline gained create rights");
    check(sys->unmount(nil, "after") == 0, "after cleanup");
    check(sys->bind("a", "replace", Sys->MBEFORE) == 0 && sys->bind("b", "replace", Sys->MAFTER) == 0, "replace setup");
    check(sys->bind("c", "replace", Sys->MREPL|Sys->MCREATE) == 0 && read("replace/token") == int 'C', "replace stack");
    (missing, missinginfo) := sys->stat("replace/a-only");
    check(missing == -1, "MREPL retained old lower layers");
    check(sys->unmount(nil, "replace") == 0, "replace cleanup");
    check(sys->unmount(nil, "mirror") == 0, "remove copied union");
    check(sys->bind("a/token", "view/token", Sys->MBEFORE) == 0 && read("view/token") == int 'T',
          "explicit file union top layer");
    check(sys->unmount(nil, "view/token") == 0 && read("view/token") == int 'V',
          "file union unmount changed containing directory");
    sys->print("PASS union order/create/readonly/copy/unmount/dynamic-mode/retained-FD-directory\n");
}
