# Copyright (c) 2026 bhgv
# EN: Real guest operations on test-injected native memory providers.
# RU: Реальные guest operations на тестовых native memory providers.
implement ProviderFD;
include "sys.m";
ProviderFD: module { init: fn(); };
sys: Sys; # Immutable builtin module after initialization.


# EN/RU: Require condition or raise label; no native handle is exposed to guest.
# Проверить condition либо raise label; guest не видит native handle.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Use separate read/write capabilities, then report completion. FD lifetime
# ends with this frame; done synchronizes only completion, not native close.
# Использовать read/write capabilities; сообщить completion. FD живут до конца
# frame; done синхронизирует completion, но не native close.
worker(done: chan of int)
{
    input := sys->open("provider-read", Sys->OREAD);
    check(input != nil, "provider read open");
    data := array[5] of byte;
    check(sys->readn(input, data, 5) == 5 && string data == "ABCDE", "provider read bytes");
    check(sys->seek(input, big 4, Sys->SEEKSTART) == big 4, "provider read seek");
    check(sys->read(input, data, 1) == 1 && int data[0] == int 'E', "provider cursor");
    check(sys->write(input, data, 1) == -1, "read capability granted write");
    (status, info) := sys->fstat(input);
    check(status == 0 && info.name == "memory-read" && info.length == big 5, "provider stat");
    (count, entries) := sys->dirread(input);
    check(count == -1 && entries == nil, "unsupported provider list");
    output := sys->open("provider-write", Sys->OWRITE);
    check(output != nil, "provider write open");
    check(sys->write(output, array[0] of byte, 0) == 0, "provider zero write");
    check(sys->read(output, data, 1) == -1, "write capability granted read");
    check(sys->write(output, array[] of {byte 'X'}, 1) == 1, "provider write");
    check(sys->seek(output, big 3, Sys->SEEKSTART) == big 3, "provider write seek");
    check(sys->write(output, array[] of {byte 'Z'}, 1) == 1, "provider overwrite");
    check(sys->fprint(output, "!") == 1, "provider formatted write");
    (status, info) = sys->fstat(output);
    check(status == 0 && info.name == "memory-write" && info.length == big 5, "provider write stat");
    directory := sys->open("provider-list", Sys->OREAD);
    check(directory != nil, "provider directory open");
    (status, info) = sys->fstat(directory);
    check(status == 0 && (info.mode & Sys->DMDIR) != 0 && info.name == "memory-dir", "provider directory stat");
    (count, entries) = sys->dirread(directory);
    check(count == 1 && len entries == 1 && entries[0].name == "entry", "provider list batch");
    (count, entries) = sys->dirread(directory);
    check(count == 0 && entries == nil, "provider list EOF");
    check(sys->read(directory, data, 1) == -1, "directory gained byte read");
    done <-= 1;
}


# EN/RU: Spawn eight independent native residents and join their guest work.
# Запустить восемь native residents и дождаться их guest work.
init()
{
    sys = load Sys Sys->PATH;
    done := chan of int;
    for (i := 0; i < 8; i++) {
        spawn worker(done);
    }
    for (i = 0; i < 8; i++) {
        <-done;
    }
    sys->print("PASS provider FD guest read/write/seek/stat/formatted/list refusal\n");
}
