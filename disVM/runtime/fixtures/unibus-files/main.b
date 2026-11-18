# Copyright (c) 2026 bhgv
# EN: Actual portable console guest uses ordinary Sys FDs for granted Unibus records, no display or host builtin.
# RU: Настоящий переносимый консольный guest использует обычные Sys FD для записей Unibus, без display/host builtin.
implement Check;
include "sys.m";
Check: module { init: fn(); };
sys: Sys; # Owner-local builtin handle, no transport authority.


# EN/RU: Decode a small positive original S-integer and return next offset, fail on truncated fixture output.
# Разобрать небольшое положительное исходное S-число, вернуть next-offset; отказ при truncated output.
integer(bytes: array of byte, at: int): (int, int)
{
    value := int bytes[at] & 63;
    while ((int bytes[at++] & 128) == 0) {
        if (at >= len bytes) {
            raise "truncated Unibus integer";
        }
        value = (value << 7) | (int bytes[at] & 127);
    }
    return (value, at);
}


# EN/RU: Read exactly one file-projected original record and verify local tag/payload; no queue or transport polling.
# Прочитать одну исходную запись из файла, проверить local tag/payload; без queue/transport polling.
take(fd: ref Sys->FD, tag: int, expected: string)
{
    bytes := array[256] of byte;
    n := sys->read(fd, bytes, len bytes);
    if (n < 3) {
        raise "missing Unibus record";
    }
    bytes = bytes[:n];
    (_, at) := integer(bytes, 0);
    (size, body) := integer(bytes, at);
    (actual, payload) := integer(bytes, body);
    if (body + size != n || actual != tag || string bytes[payload:] != expected) {
        raise "wrong Unibus record";
    }
}


# EN/RU: Open only explicitly mounted file capabilities, write independently fixed canonical frames, consume peer receipts.
# Открыть лишь явно mounted file capabilities, записать независимые фиксированные frames, получить peer receipts.
init()
{
    sys = load Sys Sys->PATH;
    seven := sys->open("/seven", Sys->ORDWR);
    nine := sys->open("/nine", Sys->OREAD);
    zero := sys->open("/zero", Sys->ORDWR);
    if (sys->open("/records", Sys->OREAD) != nil) {
        raise "raw transport leaked into application namespace";
    }
    if (seven == nil || nine == nil || zero == nil) {
        raise "missing granted Unibus files";
    }
    take(nine, 9, "nine");
    take(nine, 9, "rx-live");
    take(zero, 0, "local-in");
    take(seven, 7, "zero");
    take(seven, 7, "one");
    take(seven, 7, "two");
    take(seven, 7, "new");
    # Fixed independent original S-records: tag7/out0 and tag0/local-out, never root0.
    first := array[] of {byte 186, byte 133, byte 135, byte 111, byte 117, byte 116, byte 48};
    local := array[] of {byte 178, byte 138, byte 128, byte 108, byte 111, byte 99, byte 97, byte 108, byte 45, byte 111, byte 117, byte 116};
    if (sys->write(seven, first, len first) != len first || sys->write(zero, local, len local) != len local) {
        raise "Unibus write refused";
    }
    take(nine, 9, "guest-verified");
    sys->print("PASS real Dis guest file Unibus bidirectional records\n");
}
