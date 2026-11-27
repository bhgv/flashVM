# Copyright (c) 2026 bhgv
# EN: Exercise ordinary isolated Sys mutations against the standalone C provider.
# RU: Проверить обычные операции Sys с отдельным провайдером C.
implement Mutations;
include "sys.m";

Mutations: module { init: fn(); };


# EN: Create and unlink a mounted file while checking independent open lifetimes.
# RU: Создать и удалить файл через mount, проверив независимое время жизни fid.
init()
{
    sys := load Sys Sys->PATH;
    mode := Sys->OWRITE | Sys->OEXCL;
    path := "/device/sample";
    writer := sys->create(path, mode, 8r666);
    if (writer == nil) {
        raise sys->sprint("mounted create: %r");
    }
    if (sys->write(writer, array of byte "old", 3) != 3) {
        raise sys->sprint("created fid is not writable: %r");
    }
    if (sys->create(path, mode, 8r666) != nil ||
        sys->create("/device/readonly/sample", mode, 8r666) != nil ||
        sys->create("/device/.prog/sub/sample", mode, 8r666) != nil ||
        sys->remove("/device") != -1 ||
        sys->remove("/device/.prog") != -1) {
        raise "unexpected mounted mutation";
    }
    reader := sys->open(path, Sys->OREAD);
    if (reader == nil || sys->remove(path) != 0 ||
        sys->open(path, Sys->OREAD) != nil || sys->remove(path) != -1) {
        raise sys->sprint("mounted remove: %r");
    }
    replacement := sys->create(path, mode, 8r666);
    if (replacement == nil || sys->write(replacement, array of byte "new", 3) != 3) {
        raise sys->sprint("recreate mounted name: %r");
    }
    bytes := array[3] of byte;
    fresh := sys->open(path, Sys->OREAD);
    if (fresh == nil || sys->readn(fresh, bytes, 3) != 3 || string bytes != "new" ||
        sys->readn(reader, bytes, 3) != 3 || string bytes != "old") {
        raise "open fids did not preserve separate generations";
    }
    if (sys->remove(path) != 0 || sys->open(path, Sys->OREAD) != nil) {
        raise "replacement did not retire";
    }
    sys->print("PASS mounted Sys.create and Sys.remove against the C SDK\n");
}
