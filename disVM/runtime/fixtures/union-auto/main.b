# Copyright (c) 2026 bhgv
# EN: MAUTORWX read/write/create/load selection and real chmod re-selection.
# RU: MAUTORWX read/write/create/load selection и re-selection после chmod.
implement AutoCheck;
include "sys.m";
include "vfsflags.m";
include "provider.m";
AutoCheck: module { init: fn(); };
sys: Sys; # Immutable ordinary Sys module after startup.


# EN/RU: Fail this guest on a false condition, preserving its diagnostic label.
# Завершить guest при false, сохранив диагностическую подпись.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Read one byte from an ordinary namespace path; local FD retires on return.
# Прочитать byte по namespace path; local FD освобождается с frame.
read(path: string): int
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "auto read: " + path);
    return int bytes[0];
}


# EN/RU: Write one byte without create/truncate; local FD retires on return.
# Записать byte без create/truncate; local FD освобождается с frame.
write(path: string, value: int)
{
    fd := sys->open(path, Sys->OWRITE);
    check(fd != nil && sys->write(fd, array[] of {byte value}, 1) == 1, "auto write: " + path);
}


# EN/RU: Create one byte using ordinary Sys.create; no manual source routing.
# Создать byte через Sys.create; без ручного выбора source.
create(path: string, value: int)
{
    fd := sys->create(path, Sys->OWRITE, 8r666);
    check(fd != nil && sys->write(fd, array[] of {byte value}, 1) == 1, "auto create: " + path);
}


# EN/RU: Build explicit and classic unions, verify permissions, wait for host chmod,
# then require new selections while existing file/module capabilities stay valid.
# Собрать explicit/classic unions, проверить права, дождаться host chmod;
# новые selection меняются, существующие file/module capabilities сохраняются.
init()
{
    sys = load Sys Sys->PATH;
    check(sys->bind("a", "view", Sys->MREPL|Vfsflags->MAUTORWX) == 0, "auto bind");
    check(sys->bind("b", "view", Sys->MAFTER) == 0 && sys->bind("c", "view", Sys->MAFTER) == 0, "auto lower bind");
    check(sys->pctl(Sys->FORKNS, nil) > 0, "auto namespace copy");
    check(sys->bind("view", "mirror", Sys->MREPL|Vfsflags->MAUTORWX) == 0, "auto alias union");
    check(sys->bind("a", "plain", Sys->MREPL) == 0 && sys->bind("b", "plain", Sys->MAFTER) == 0, "classic bind");
    check(read("view/common") == int 'A', "read did not select top readable");
    check(read("view/masked") == int 'B', "owner bits confused with other read");
    check(sys->open("plain/common", Sys->OWRITE) == nil, "classic semantics changed");
    check(sys->open("view/denied", Sys->OREAD) == nil && sys->open("view/denied", Sys->OWRITE) == nil, "all-denied selection");
    held := sys->open("view/common", Sys->OREAD);
    check(held != nil, "retained auto FD");
    write("view/common", int 'W');
    check(read("view/common") == int 'A', "write changed read layer");
    create("view/create/new", int 'N');
    check(sys->create("view/create/exclusive", Sys->OWRITE|Sys->OEXCL, 8r666) == nil, "exclusive collision retried");
    old := load NamespaceProvider "view/tool.dis";
    check(old != nil && old->tag() == 22, "load did not require read plus execute");
    check(read("mirror/common") == int 'A', "copied auto policy lost");
    sys->print("AUTO_CHMOD_READY\n");
    bytes := array[1] of byte;
    check(sys->read(sys->fildes(0), bytes, 1) == 1, "auto chmod release");
    check(read("view/common") == int 'W', "read permissions not refreshed");
    write("view/common", int 'Z');
    create("view/create/upper", int 'U');
    create("view/create/lower-existing", int 'L');
    check(sys->create("view/create/collision", Sys->OWRITE|Sys->OEXCL, 8r666) == nil, "upper exclusive collision retried");
    (status, metadata) := sys->stat("view/common");
    check(status == 0 && metadata.mode == 8r200, "stat silently became read selection");
    check(sys->read(held, bytes, 1) == 1 && int bytes[0] == int 'Z', "open FD did not retain original node");
    fresh := load NamespaceProvider "view/tool.dis";
    check(fresh != nil && fresh->tag() == 11 && old->tag() == 22, "execute selection/cache not refreshed");
    check(read("mirror/common") == int 'W', "alias policy ignored new permissions");
    check(sys->unmount(nil, "view") == 0 && sys->unmount(nil, "mirror") == 0 && sys->unmount(nil, "plain") == 0, "auto cleanup");
    sys->print("PASS MAUTORWX read/write/create/search/execute/chmod/cache/classic/retained\n");
}
