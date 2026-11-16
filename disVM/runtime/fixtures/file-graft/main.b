# Copyright (c) 2026 bhgv
# EN: Single-file direct/prepared publication, mount/plain bind/Vfsctl into a populated directory.
# RU: Single-file direct/prepared publication, mount/plain bind/Vfsctl в заполненный каталог.
implement Check;
include "sys.m";
include "vfspublish.m";
include "vfsctl.m";
Check: module { init: fn(); };
sys: Sys; # Ordinary filesystem/FD builtin.
api: Vfsexport; # Explicit publication and chosen-root capabilities.
pub: Vfspublish; # One-request preparation adapter.
ctl: Vfsctl; # Indexed bind/move API over the same namespace stacks.


# EN/RU: Fail each visible guest contract explicitly, including in JIT builds.
# Явно завершить каждый visible guest contract, включая JIT builds.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Read a file through ordinary Sys.open/read, never through fixture bypasses.
# Прочитать файл через ordinary Sys.open/read, без fixture bypass.
value(path: string): string
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "file read " + path);
    return string bytes;
}


# EN/RU: Require one visible entry per bound leaf and preserve both unrelated native files.
# Требовать по одной visible entry на bound leaf и сохранить оба unrelated native files.
listing(fd: ref Sys->FD, grafts: int)
{
    check(fd != nil, "open containing directory");
    imported := copied := prepared := plain := automatic := neighbour := collision := 0;
    for (;;) {
        (count, entries) := sys->dirread(fd);
        check(count >= 0, "containing directory read");
        if (!count) {
            break;
        }
        for (i := 0; i < count; i++) {
            case entries[i].name {
            "imported" => imported++;
            "copied" => copied++;
            "prepared" => prepared++;
            "plain" => plain++;
            "auto" => automatic++;
            "neighbour" => neighbour++;
            "collision" => collision++;
            }
        }
    }
    check(imported == grafts && copied == grafts && prepared == grafts && plain == grafts && automatic == grafts &&
          neighbour == 1 && collision == 1, "graft listing hid/duplicated sibling names");
}


# EN/RU: Test direct and per-request prepared single-file grants through the actual guest ABI.
# Проверить direct и per-request prepared single-file grants через actual guest ABI.
init()
{
    sys = load Sys Sys->PATH;
    api = load Vfsexport Vfsexport->PATH;
    pub = load Vfspublish Vfspublish->PATH;
    ctl = load Vfsctl Vfsctl->PATH;
    check(sys != nil && api != nil && pub != nil && ctl != nil, "load file publication APIs");
    pub->init();
    ctl->init();
    early := sys->open("out", Sys->OREAD);
    check(early != nil, "directory open before grafts");
    a := api->capture("data/a", 0, Vfsexport->Read);
    b := api->capture("data/b", 0, Vfsexport->Read);
    check(a != nil && b != nil, "capture exact files");
    direct := api->announce(4, 0, a);
    ticket := api->submit(api->endpoint(direct), "one-file", 1, Vfsexport->Read);
    check(ticket != nil && api->status(ticket) == Vfsexport->Granted, "direct file announce");
    root := api->take(ticket);
    (status, nil, directory) := api->info(root);
    check(status == 0 && !directory && api->mount(root, "out/imported", 0, 0) == 0,
          "direct file mount to new leaf");
    api->cancel(ticket);
    check(value("out/imported") == "A" && value("out/neighbour") == "N", "mount covered parent");
    check(sys->bind("data/b", "out/plain", Sys->MBEFORE) == 0 && value("out/plain") == "B",
          "plain Sys.bind to new leaf");
    check(ctl->bind("out/imported", "out/copied", Vfsctl->MBEFORE, 0) == 0 &&
          ctl->bind("data/b", "out/copied", Vfsctl->MINS, -1) == 0 && value("out/copied") == "A",
          "Vfsctl file bind/indexed order");
    prepared := api->announce(4, Vfsexport->Prepare, nil);
    ticket = api->submit(api->endpoint(prepared), "requested-file", 1, Vfsexport->Read);
    request := pub->next(prepared);
    check(request != nil && request.kind == 1 && pub->reply(prepared, request, b), "prepared file substitution");
    chosen := api->take(ticket);
    check(chosen != nil && api->mount(chosen, "out/prepared", Vfsctl->MAFTER, 0) == 0 &&
          value("out/prepared") == "B", "prepared single-file mount");
    api->cancel(ticket);
    check(api->mount(root, "out/collision", Vfsctl->MAFTER, 0) == 0 && value("out/collision") == "C",
          "file MAFTER ignored native layer");
    check(api->mount(b, "out/collision", Vfsctl->MINS, 0) == 0 && value("out/collision") == "B",
          "file MINS did not select top layer");
    (found, revision, layers) := ctl->layers("out/collision");
    check(found == Vfsctl->Ready && len layers == 3 &&
          ctl->move("out/collision", revision, (hd layers).id, -1) == 0 && value("out/collision") == "C",
          "file layer inspection/move");
    writeroot := api->capture("data/w", 0, Vfsexport->Read|Vfsexport->Write);
    check(writeroot != nil && api->mount(a, "out/auto", Vfsctl->MAUTORWX, 0) == 0 &&
          api->mount(writeroot, "out/auto", Vfsctl->MAFTER, 0) == 0, "automatic file stack setup");
    output := sys->open("out/auto", Sys->OWRITE);
    check(output != nil && sys->write(output, array[] of {byte 'Z'}, 1) == 1 &&
          value("out/auto") == "A" && value("data/w") == "Z", "automatic file read/write layer selection");
    listing(early, 1);
    listing(sys->open("out", Sys->OREAD), 1);
    check(api->mount(root, "missing/file", 0, 0) < 0 && api->mount(root, "out", 0, 0) < 0,
          "missing parent/directory replaced by file");
    held := sys->open("out/imported", Sys->OREAD);
    check(held != nil && api->close(direct) && api->close(prepared), "held file/publication close");
    for (paths := list of {"out/imported", "out/copied", "out/prepared", "out/plain", "out/collision", "out/auto"};
         paths != nil; paths = tl paths) {
        check(sys->unmount(nil, hd paths) == 0, "unmount exact file leaf");
    }
    listing(sys->open("out", Sys->OREAD), 0);
    bytes := array[1] of byte;
    check(sys->read(held, bytes, 1) == 1 && string bytes == "A" && value("out/neighbour") == "N" &&
          value("out/collision") == "C", "unmount destroyed handle/sibling/native layer");
    sys->print("PASS single-file publication mount Sys.bind Vfsctl listing siblings layers unmount\n");
}
