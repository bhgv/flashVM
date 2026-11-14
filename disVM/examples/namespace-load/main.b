# Copyright (c) 2026 bhgv
# EN: Concurrent file loads share virtual names without sharing namespace identity.
# RU: Параллельный file load использует общие virtual names, не смешивая namespace.
implement NamespaceLoadDemo;
include "sys.m";
include "processctl.m";
include "provider.m";
NamespaceLoadDemo: module { init: fn(); };
sys: Sys; # Immutable builtin loaded before spawning.
control: Processctl; # Child completion identity and retirement.
Workers: con 8; # Concurrent owners with detached namespace groups.
Rounds: con 24; # Repeated code-cache lookups with private module instances.


# EN/RU: Require condition or raise label; no resources transferred.
# Проверить condition либо raise label; без передачи ресурсов.
require(condition: int, label: string)
{
    if (!condition) {
        raise label;
    }
}


# EN: Detach namespace, bind one of two providers at the same virtual path and
# synchronize start; verify repeated loads and private MP across native waits.
# RU: Отделить namespace, связать один из двух providers по одному virtual path
# и синхронизировать start; проверить повторные load и private MP при native waits.
worker(id: int, ready, start, done: chan of int)
{
    require(sys->pctl(Sys->FORKNS, nil) > 0, "loader FORKNS failed");
    source := "package/source-a";
    expected := 11;
    if (id % 2) {
        source = "package/source-b";
        expected = 22;
    }
    require(sys->bind(source, "package/visible", Sys->MREPL) == 0, "loader bind failed");
    ready <-= control->pid();
    <-start;
    for (round := 0; round < Rounds; round++) {
        provider := load NamespaceProvider "visible/provider.dis";
        require(provider != nil, "namespace provider missing");
        require(provider->tag() == expected, "namespace code cache crossed groups");
        value := id * Rounds + round;
        require(provider->run(value) == expected + value, "namespace module MP crossed instances");
    }
    done <-= control->pid();
}


# EN/RU: Rebase a detached root; relative/absolute load works and package escape fails.
# Сменить detached root; relative/absolute load работают, package escape запрещён.
rooted(done: chan of int)
{
    require(sys->pctl(Sys->FORKNS, nil) > 0, "root loader FORKNS failed");
    require(sys->chdir("package/source-a") == 0, "root loader chdir failed");
    relative := load NamespaceProvider "provider.dis";
    require(relative != nil && relative->tag() == 11, "chdir did not update import directory");
    require(sys->pctl(Sys->NEWNS, nil) > 0, "root loader NEWNS failed");
    relative = load NamespaceProvider "provider.dis";
    absolute := load NamespaceProvider "/provider.dis";
    escaped := load NamespaceProvider "/dis/lib/bufio.dis";
    require(relative != nil && absolute != nil && relative->tag() == 11 && absolute->tag() == 11,
        "NEWNS module root changed");
    require(escaped == nil, "NEWNS loader bypassed root via bundled fallback");
    done <-= 1;
}


# EN: Launch from a directory containing package/main.dis; imports start beside
# that entry while ordinary Sys paths use caller cwd. Check unified bundled paths,
# retained old modules and eight independent loading owners.
# RU: Запускать из directory с package/main.dis; начальные imports рядом с entry,
# Sys paths используют caller cwd. Проверить unified bundled paths, сохранность
# старых модулей и восемь независимых owners, загружающих модули.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    initial := load NamespaceProvider "source-a/provider.dis";
    bundled := load NamespaceProvider "/dis/lib/bufio.dis";
    require(initial != nil && initial->tag() == 11, "initial import base changed");
    require(bundled != nil && bundled->tag() == 11, "initial bundled binding missing");
    (status, info) := sys->stat("/dis/lib/bufio.dis");
    require(status == 0 && info.length > big 0, "Sys stat bypassed bundled namespace");
    fd := sys->open("/dis/lib/bufio.dis", Sys->OREAD);
    require(fd != nil, "Sys open bypassed bundled namespace");
    fd = nil;
    require(sys->bind("package/source-b/provider.dis", "/dis/lib/bufio.dis", Sys->MREPL) == 0,
        "bundled file bind failed");
    changed := load NamespaceProvider "/dis/lib/bufio.dis";
    require(changed != nil && changed->tag() == 22 && bundled->tag() == 11, "bundled rebind changed old module");
    missing := load NamespaceProvider "missing-provider.dis";
    invalid := load NamespaceProvider "source-a/provider.dis\u0000suffix";
    invalidbuiltin := load Sys "$Sys\u0000suffix";
    invalidself := load NamespaceLoadDemo "$self\u0000suffix";
    nilname: string;
    empty := load NamespaceProvider nilname;
    require(missing == nil && invalid == nil && invalidbuiltin == nil && invalidself == nil && empty == nil,
        "invalid load admitted a path alias");
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    for (id := 0; id < Workers; id++) {
        spawn worker(id, ready, start, done);
    }
    for (id = 0; id < Workers; id++) {
        <-ready;
    }
    for (id = 0; id < Workers; id++) {
        start <-= 1;
    }
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        result := control->killwait(pid);
        require(result == Processctl->Retired || result == Processctl->Denied, "loader child retained");
    }
    spawn rooted(done);
    require(<-done == 1, "rooted loader failed");
    require(initial->tag() == 11 && bundled->tag() == 11, "peer loading changed retained modules");
    sys->print("NAMESPACELOAD workers=8 loads=192 providers=11,22 root=11 old=11 bundled=22\n");
}
