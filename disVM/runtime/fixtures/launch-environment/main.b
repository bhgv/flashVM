# Copyright (c) 2026 bhgv
# EN: Application init sees prepared client/server union and inspects/reorders one device.
# RU: Application init видит prepared client/server union и меняет порядок одного device.
implement Check;
include "sys.m";
include "vfsctl.m";
Check: module { init: fn(); };
sys: Sys; # Existing builtin FD/namespace interface.
ctl: Vfsctl; # Existing layer API plus copied launch provenance.


# EN/RU: Raise explicit actual interpreter/JIT contract mismatch.
# Raise при нарушении настоящего interpreter/JIT contract.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Read exactly one ordinary device byte, keeping the FD lifetime local.
# Прочитать byte обычного device; FD принадлежит local call.
value(path: string): string
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "device read " + path);
    return string bytes;
}


# EN/RU: Execute only after bootstrap prepared the isolated namespace, never download provider code.
# Выполнить после подготовки isolated namespace; provider code не скачивается.
appinit()
{
    check(value("/dev/camera") == "T" && value("/server") == "P", "initial terminal-first namespace");
    check(sys->open("/denied", Sys->OREAD) == nil && sys->open("/etc/passwd", Sys->OREAD) == nil,
          "denial or missing path exposed fallback");
    old := sys->open("/dev/camera", Sys->OREAD);
    (status, revision, layers) := ctl->layers("/dev/camera");
    check(status == Vfsctl->Ready && len layers == 2, "effective camera stack");
    first, second, stale: ref Vfsctl->Origin;
    secondstatus: int;
    (status, first) = ctl->origin("/dev/camera", revision, 0);
    (secondstatus, second) = ctl->origin("/dev/camera", revision, -1);
    check(status == Vfsctl->Ready && secondstatus == Vfsctl->Ready && first != nil && second != nil &&
          first.role == Vfsctl->Terminal && second.role == Vfsctl->ExecutionProvider &&
          first.nickname == "terminal" && second.nickname == "provider" &&
          first.systemid != second.systemid && first.sessionid == second.sessionid, "launch source metadata");
    check(ctl->move("/dev/camera", revision, (hd layers).id, -1) == 0 &&
          value("/dev/camera") == "P" && value("/dev/keyboard") == "T", "camera-only reorder");
    (status, stale) = ctl->origin("/dev/camera", revision, 0);
    check(status == Vfsctl->Stale && stale == nil, "stale provenance accepted");
    (status, revision, nil) = ctl->layers("/dev/camera");
    (status, first) = ctl->origin("/dev/camera", revision, 0);
    check(status == Vfsctl->Ready && first.role == Vfsctl->ExecutionProvider, "move lost source identity");
    bytes := array[1] of byte;
    check(sys->read(old, bytes, 1) == 1 && string bytes == "T", "open FD followed reorder");
    check(sys->bind("/dev/camera", "/saved", 0) == 0 && value("/saved") == "P", "bound camera provenance");
    (status, revision, nil) = ctl->layers("/saved");
    (status, first) = ctl->origin("/saved", revision, 0);
    check(status == Vfsctl->Ready && first.role == Vfsctl->ExecutionProvider, "bind lost source identity");
    check(sys->unmount(nil, "/") == 0 && sys->open("/etc/passwd", Sys->OREAD) == nil &&
          sys->chdir("/") == -1 && sys->pctl(Sys->NEWNS, nil) == -1 && value("/saved") == "P",
          "removing root grants exposed native host paths");
    sys->print("PASS guest prepared launch environment origins and device moves\n");
}


# EN/RU: Bootstrap loads local interfaces first, then installs SDK-prepared namespace before appinit.
# Bootstrap загружает local interfaces, ставит prepared namespace и вызывает appinit.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Vfsctl Vfsctl->PATH;
    ctl->init();
    check(sys->pctl(Sys->NEWNS, nil) > 0, "prepare before application init");
    appinit();
}
