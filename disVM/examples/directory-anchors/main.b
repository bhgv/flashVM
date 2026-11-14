# Copyright (c) 2026 bhgv
# EN: Four native owners keep cwd, imports and directory cursors through rebind.
# RU: Четыре native owners сохраняют cwd/imports/directory cursors при rebind.
implement DirectoryAnchors;
include "sys.m";
include "processctl.m";
include "provider.m";
DirectoryAnchors: module { init: fn(); };
sys: Sys; # Shared immutable builtin interface.
control: Processctl; # Joined child lifetimes.
Rename: con 0; # Development driver enables real host rename/decoy replacement.
Workers: con 4; # Leaves descriptor headroom for independent cursors and native loads.


# EN/RU: Require condition or raise label; no native resource ownership transfer.
# Проверить condition либо raise label; без передачи native resources.
require(condition: int, label: string)
{
    if (!condition) {
        raise label;
    }
}


# EN/RU: Read one byte, letting the rooted local FD retire after return.
# Прочитать byte; локальный rooted FD освобождается после return.
read(path: string): int
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    require(fd != nil && sys->read(fd, bytes, 1) == 1, "anchor token read failed");
    return int bytes[0];
}


# EN/RU: Require token metadata from this independent cursor's first batch.
# Требовать metadata token в первом batch независимого cursor.
cursor(fd: ref Sys->FD)
{
    (count, entries) := sys->dirread(fd);
    found := 0;
    for (index := 0; index < count; index++) {
        if (entries[index].name == "token" && entries[index].length == big 1) {
            found = 1;
        }
    }
    require(found, "directory cursor redirected or shared its offset");
}


# EN: Detach before chdir, hold two independent cursors and wait for controller.
# A covering bind must change future absolute routing without changing this cwd.
# RU: Отделиться до chdir, держать два cursors, ждать controller. Covering bind
# меняет будущий absolute routing, но не этот cwd. ready/start/done/results rooted.
worker(id: int, ready, start, done, results: chan of int)
{
    require(sys->pctl(Sys->FORKNS, nil) > 0 && sys->chdir("visible") == 0, "anchor owner setup failed");
    first := sys->open(".", Sys->OREAD);
    second := sys->open(".", Sys->OREAD);
    require(first != nil && second != nil, "anchor cursors missing");
    ready <-= control->pid();
    <-start;
    value := read("token");
    bad := value != int 'A';
    if (bad) {
        sys->print("DETECTED directory path fallback value=%d\n", value);
    }
    if (Rename) {
        decoy := load NamespaceProvider "../source-a/provider.dis";
        require(decoy != nil && decoy->tag() == 22, "real host decoy was not installed");
    }
    require(sys->bind("../other", ".", Sys->MREPL) == 0, "covering anchor rebind failed");
    require(read("token") == int 'A', "covering rebind redirected held cwd");
    provider := load NamespaceProvider "provider.dis";
    require(provider != nil && provider->tag() == 11, "held import directory redirected");
    output := sys->create("created-" + string id, Sys->OWRITE, 8r666);
    bytes := array[] of {byte 'C'};
    require(output != nil && sys->write(output, bytes, 1) == 1, "held directory create failed");
    output = nil;
    cursor(first);
    cursor(second);
    first = nil;
    second = nil;
    require(sys->chdir("..") == 0 && read("visible/token") == int 'B', "future namespace route not updated");
    require(sys->unmount("other", "visible") == 0, "identity-filtered unmount failed");
    require(sys->unmount(nil, "other") == 0, "last directory binding removal failed");
    require(provider->tag() == 11, "resource release changed retained module");
    done <-= control->pid();
    results <-= bad;
}


# EN: Own original bindings, copy them to four children, then remove parent refs
# before children run. Optional stdin permit lets the driver rename real dirs.
# RU: Владеть исходными bindings, передать копии четырём children, удалить свои
# ссылки до их работы. Optional stdin permit позволяет driver переименовать каталоги.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    require(sys->bind("source-a", "visible", Sys->MREPL) == 0, "initial A bind failed");
    require(sys->bind("source-b", "other", Sys->MREPL) == 0, "initial B bind failed");
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    results := chan of int;
    for (id := 0; id < Workers; id++) {
        spawn worker(id, ready, start, done, results);
    }
    for (id = 0; id < Workers; id++) {
        <-ready;
    }
    require(sys->unmount(nil, "visible") == 0 && sys->unmount(nil, "other") == 0, "parent refs retained");
    if (Rename) {
        sys->print("ANCHORSREADY workers=4\n");
        bytes := array[1] of byte;
        require(sys->read(sys->fildes(0), bytes, 1) == 1, "host rename permit missing");
    }
    for (id = 0; id < Workers; id++) {
        start <-= 1;
    }
    bad := 0;
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        bad += <-results;
        status := control->killwait(pid);
        require(status == Processctl->Retired || status == Processctl->Denied, "anchor owner retained");
    }
    sys->print("ANCHORSDONE workers=4 bad=%d cwd=A route=B module=11\n", bad);
}
