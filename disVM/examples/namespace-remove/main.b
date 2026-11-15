# Copyright (c) 2026 bhgv
# EN: Native owners remove held-namespace entries while opened files/modules survive.
# RU: Native owners удаляют entries удержанного namespace, сохраняя открытые файлы/модули.
implement NamespaceRemove;
include "sys.m";
include "processctl.m";
include "provider.m";
NamespaceRemove: module { init: fn(); };
sys: Sys; # Immutable builtin interface shared by guest owners.
ctl: Processctl; # PID and joined native child lifetime.
Workers: con 4; # Concurrent permanent OS owners.
Rename: con 0; # Driver variant performs actual host rename before removal.


# EN/RU: Require condition with ordinary error, without native resource transfer.
# Проверить condition обычной ошибкой, без передачи native resource.
require(value: int, label: string)
{
    if (!value) { raise label; }
}


# EN: Detach cwd, retain opened victim/module, then delete original child names.
# Rooted FDs/Modlinks remain usable. The shared result cell exercises guarded
# scalar publication; only return publication may retry, never deletion itself.
# RU: Отделить cwd, удержать открытые victim/module, удалить исходные имена.
# Rooted FDs/Modlinks продолжают работать. Shared result cell проверяет guarded
# publication; повторяется только публикация, не удаление.
worker(id: int, ready, start, done, bads, wins: chan of int, result: array of int)
{
    require(sys->pctl(Sys->FORKNS, nil) > 0 && sys->chdir("visible") == 0, "remove owner setup failed");
    name := "victim-" + string id;
    fd := sys->open(name, Sys->OREAD);
    require(fd != nil, "victim open failed");
    image := "provider-" + string id + ".dis";
    provider := load NamespaceProvider image;
    require(provider != nil && provider->tag() == 11, "remove provider load failed");
    ready <-= ctl->pid();
    <-start;
    require(sys->remove(name) == 0, "initial removal failed");
    result[0] = sys->remove("publication-" + string id);
    (status, ignored) := sys->stat(name);
    bad := status == 0;
    if (bad) { sys->print("DETECTED remove path fallback owner=%d\n", id); }
    bytes := array[1] of byte;
    require(sys->read(fd, bytes, 1) == 1 && bytes[0] == byte 'A', "unlink invalidated opened FD");
    fd = nil;
    if (!bad) { require(sys->remove(name) == -1, "second remove succeeded"); }
    require(sys->remove(image) == 0, "module image removal failed");
    missing := load NamespaceProvider image;
    require(missing == nil && provider->tag() == 11, "unlink invalidated module or stale cache loaded missing file");
    require(sys->remove("empty-" + string id) == 0, "empty directory removal failed");
    full := "full-" + string id;
    require(sys->remove(full) == -1, "nonempty directory recursively removed");
    require(sys->remove(full + "/keep") == 0 && sys->remove(full) == 0, "directory cleanup failed");
    source := "alias-source-" + string id;
    target := "alias-target-" + string id;
    require(sys->bind(source, target, Sys->MREPL) == 0, "file alias bind failed");
    require(sys->remove(target) == 0 && sys->unmount(nil, target) == 0, "file alias removal failed");
    (status, ignored) = sys->stat(target);
    require(status == 0, "alias removal deleted original target");
    nul := array[] of {byte 0};
    require(sys->remove(".") == -1 && sys->remove("/") == -1 &&
        sys->remove("bad" + string nul + "path") == -1, "invalid remove path accepted");
    winner := sys->remove("one-winner") == 0;
    done <-= ctl->pid();
    bads <-= bad;
    wins <-= winner;
}


# EN/RU: Own initial binding, start four copied owners, permit optional rename,
# then join and require one shared-name removal winner. Channel values are scalar.
# Владеть binding, запустить четыре owners, разрешить optional rename; join и
# требовать одного победителя удаления общего имени. В каналах только scalar.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Processctl Processctl->PATH;
    require(sys->bind("source-a", "visible", Sys->MREPL|Sys->MCREATE) == 0, "initial remove bind failed");
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    bads := chan of int;
    wins := chan of int;
    result := array[1] of int;
    for (id := 0; id < Workers; id++) { spawn worker(id, ready, start, done, bads, wins, result); }
    for (id = 0; id < Workers; id++) { <-ready; }
    if (Rename) {
        sys->print("REMOVEREADY workers=4\n");
        permit := array[1] of byte;
        require(sys->read(sys->fildes(0), permit, 1) == 1, "remove rename permit missing");
    }
    for (id = 0; id < Workers; id++) { start <-= 1; }
    bad := 0;
    winners := 0;
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        bad += <-bads;
        winners += <-wins;
        status := ctl->killwait(pid);
        require(status == Processctl->Retired || status == Processctl->Denied, "remove owner retained");
    }
    require(winners == 1, "concurrent remove winner mismatch");
    sys->print("REMOVEDONE workers=4 bad=%d winners=1 file=A module=11\n", bad);
}
