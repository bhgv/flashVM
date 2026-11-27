# Copyright (c) 2026 bhgv
# EN: Publish real Limbo-owned fields, observe AS changes, then replace the entire VFS record.
# RU: Опубликовать поля Limbo, увидеть изменения AS и заменить запись VFS целиком.
implement Client;
include "sys.m";
include "vfsexport.m";

Client: module { init: fn(); };
sys: Sys; # Local system I/O and rendezvous with the movie.
exports: Vfsexport; # Native capability constructors and mount operations.
Directory: con "/live/.var"; # Mounted at the same path in both generations.

include "named_movie_upload.b";


# EN: Wait for a positive marker from the movie, with a bounded startup budget.
# RU: Дождаться положительной метки фильма с ограниченным сроком.
await(path: string)
{
    began := sys->millisec();
    while (sys->millisec() - began < 5000) {
        (ok, nil) := sys->stat(path);
        if (ok == 0) {
            return;
        }
        sys->sleep(5);
    }
    raise "missing live-stage marker " + path;
}


# EN: Construct two aliased Int32 positions, a nested record and a seeded big vector.
# RU: Создать два алиаса Int32, вложенную запись и вектор big с одним элементом.
publish(cell: ref Vfsexport->Root): ref Vfsexport->Root
{
    numbers := exports->intarray(cell :: exports->intvar(19) :: cell :: nil);
    text := exports->stringvar("start");
    vector := exports->bigvector();
    seed := exports->open(vector, ".push", Sys->OWRITE);
    ten := array[] of {byte 0, byte 0, byte 0, byte 0, byte 0, byte 0, byte 0, byte 10};
    if (seed == nil || sys->write(seed, ten, len ten) != len ten) {
        raise "cannot seed big vector";
    }
    seed = nil;
    magnitude := exports->bigvar(big "-9223372036854775808");
    details := exports->record(("name", text) :: ("wide", vector) :: ("magnitude", magnitude) :: nil);
    root := exports->record(("numbers", numbers) :: ("details", details) :: ("scalar", cell) :: nil);
    if (root == nil || exports->mount(root, Directory, Sys->MREPL, 0) != 0) {
        raise "cannot publish live record";
    }
    return root;
}


# EN: Keep the original storage rooted for validation even after replacing its published name.
# RU: Удерживать исходные данные для проверки даже после замены опубликованного имени.
init()
{
    sys = load Sys Sys->PATH;
    exports = load Vfsexport Vfsexport->PATH;
    original := exports->intvar(-17);
    root := publish(original);
    start("live-stage.swf");
    await("/live-stage-mutated");
    (ok, value) := exports->intget(original);
    if (!ok || value != 42 || root == nil) {
        raise "AS write did not reach the original native Int32 cell";
    }
    await("/live-stage-held");
    if (sys->unmount(nil, Directory) != 0) {
        raise "cannot retire the first publication";
    }
    fresh := publish(exports->intvar(99));
    gate := sys->open("/live-stage-gate", Sys->OWRITE);
    permit := array[] of {byte '1'};
    if (fresh == nil || gate == nil || sys->write(gate, permit, len permit) != len permit) {
        raise "cannot signal live replacement";
    }
    await("/live-stage-done");
    (ok, value) = exports->intget(original);
    if (!ok || value != 42) {
        raise "stale AS write reached retired storage";
    }
    sys->print("PASS native live-stage RPC paired VMs\n");
}
