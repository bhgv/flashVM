# Copyright (c) 2026 bhgv
# EN: Real provider cwd identity, namespace copy, module loading and automatic events.
# RU: Настоящие provider cwd identity, namespace copy, module loading и automatic events.
implement Check;
include "sys.m";
include "value.m";
Check: module { init: fn(); };
sys: Sys; # Immutable builtin module shared by permanent guest threads.


# EN/RU: Require interpreter/JIT behaviour explicitly; raise on mismatch.
# Проверить interpreter/JIT behaviour; raise при mismatch.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Return one ordinary file byte; path is resolved through the current namespace.
# Вернуть byte обычного файла; path разрешается через current namespace.
value(path: string): string
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[1] of byte;
    check(fd != nil && sys->read(fd, bytes, 1) == 1, "cwd read " + path);
    return string bytes;
}


# EN/RU: Drain one-record system reads until the native inbox reports AGAIN; return VFS kinds.
# Читать по одной записи до AGAIN native inbox, вернуть VFS kinds.
events(fd: ref Sys->FD): int
{
    bytes := array[9000] of byte;
    classes := 0;
    for (i := 0; i < 64; i++) {
        n := sys->read(fd, bytes, len bytes);
        if (n < 0 && sys->sprint("%r") == "native input temporarily unavailable") {
            return classes;
        }
        check(n > 0, "cwd event read");
        (count, words) := sys->tokenize(string bytes[:n], " \n");
        check(count == 8 && hd words == "system", "cwd event frame");
        words = tl words;
        if (hd words == "1") {
            classes |= int hd tl words;
        }
    }
    raise "cwd event queue failed to drain";
    return 0;
}


# EN/RU: Copy namespace/cwd on a permanent native child, then change only the copied cwd.
# Скопировать namespace/cwd в native child, изменить только copied cwd.
child(base: string, done: chan of int)
{
    check(sys->pctl(Sys->FORKNS, nil) > 0 && value("item") == "A", "copied provider cwd");
    check(sys->chdir(base+"/plain") == 0 && value("item") == "C", "child native cwd");
    loaded := load Value "provider.dis";
    check(loaded != nil && loaded->value() == 73, "child import base");
    done <-= 1;
}


# EN/RU: Preserve acquired provider after source replacement/unmount; observe live child data.
# Сохранить acquired provider после replacement/unmount; children остаются live.
init()
{
    sys = load Sys Sys->PATH;
    base := "@FIXTURE_BASE@";
    control := sys->open("remote/ctl", Sys->OWRITE);
    system := sys->open("/dev/sysevents", Sys->OREAD);
    check(sys->chdir("remote/flip") == -1 && value("plain/item") == "C",
          "capture changed directory kind");
    check(base != "" && control != nil && system != nil && sys->chdir("remote/dir") == 0,
          "provider chdir admission");
    check(value("item") == "A", "initial provider cwd");
    check(sys->write(control, array[] of {byte 'B'}, 1) == 1 &&
          value("item") == "A" && value(base+"/remote/dir/item") == "B", "captured cwd identity");
    check(sys->bind(base+"/plain", base+"/remote", 0) == 0 &&
          value("item") == "A" && value(base+"/remote/item") == "C", "covering bind changed cwd");
    loaded := load Value "provider.dis";
    check(loaded == nil, "provider module refusal reopened a native path");
    check(sys->unmount(nil, base+"/remote") == 0 && value("item") == "A", "unmount dropped cwd");
    events(system);
    check(sys->write(control, array[] of {byte 'P'}, 1) == 1 && (events(system)&4) != 0,
          "unmounted held cwd lost automatic provider events");
    done := chan of int;
    spawn child(base, done);
    check(<-done == 1 && value("item") == "A", "child cwd escaped copied namespace");
    check(sys->chdir("item") == -1 && value("item") == "A", "file chdir changed cwd");
    check(sys->chdir(base+"/plain") == 0 && value("item") == "C", "restore native cwd");
    events(system);
    check(sys->write(control, array[] of {byte 'P'}, 1) == 1 && events(system) == 0,
          "departed cwd still subscribed to hidden source");
    check(sys->bind(base+"/plain", base+"/remote", 0) == 0 &&
          sys->bind(base+"/plain", base+"/remote", Sys->MAFTER) == 0 &&
          sys->chdir(base+"/remote") == -1 && value("item") == "C", "union cwd refusal changed cwd");
    sys->print("PASS guest provider cwd identity copy loader and events\n");
}
