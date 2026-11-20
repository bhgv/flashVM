# Copyright (c) 2026 bhgv
# EN: Actual portable console guest uses ordinary Sys FDs for granted Unibus records, no display or host builtin.
# RU: Настоящий переносимый консольный guest использует обычные Sys FD для записей Unibus, без display/host builtin.
implement Check;
include "sys.m";
include "value.m";
include "launchcontext.m";
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


# EN/RU: Read the retained file using an ordinary local Sys FD and verify its access.
# Прочитать удерживаемый файл обычным локальным Sys FD и проверить права.
checkheld()
{
    if (sys->open("/held-input", Sys->OREAD) != nil ||
        sys->open("/input", Sys->ORDWR) != nil) {
        raise "opened undelegated held input mode or path";
    }
    input := sys->open("/input", Sys->OREAD);
    if (input == nil || sys->open("/input", Sys->OREAD) != nil) {
        raise "held input was not granted exactly once";
    }
    bytes := array[32] of byte;
    n := sys->read(input, bytes, len bytes);
    if (n <= 0 || string bytes[:n] != "from-held-cursor" ||
        sys->read(input, bytes, len bytes) != 0) {
        raise "held input lost its initial cursor or EOF";
    }
    if (sys->open("/output", Sys->OREAD) != nil) {
        raise "output grant widened to read";
    }
    output := sys->open("/output", Sys->OWRITE);
    payload := array of byte "written-with-held-fd";
    if (output == nil || sys->write(output, payload, len payload) != len payload) {
        raise "held output refused";
    }
    sys->print("PASS transferred Sys FD retained source position\n");
}


# EN/RU: Open only explicitly mounted file capabilities, write independently fixed canonical frames, consume peer receipts.
# Открыть лишь явно mounted file capabilities, записать независимые фиксированные frames, получить peer receipts.
init()
{
    sys = load Sys Sys->PATH;
    launch := load Launchcontext Launchcontext->PATH;
    if (launch == nil || launch->name() != "provider/unibus-files.dis") {
        raise "missing accepted launch identity";
    }
    arguments := launch->arguments();
    if (len arguments != 3 || arguments[0] != byte 'r' ||
        arguments[1] != byte 0 || arguments[2] != byte 'n') {
        raise "wrong accepted startup bytes";
    }
    app := load Value "/app.dis";
    if (app == nil || app->value() != 111) {
        raise "actual Styx module/dependency load failed";
    }
    checkheld();
    seven := sys->open("/seven", Sys->ORDWR);
    nine := sys->open("/nine", Sys->OREAD);
    zero := sys->open("/zero", Sys->ORDWR);
    if (sys->open("/records", Sys->OREAD) != nil || sys->open("/launch", Sys->OREAD) != nil ||
        sys->open("/job-records", Sys->OREAD) != nil || sys->open("/job-request", Sys->OREAD) != nil) {
        raise "raw transport leaked into application namespace";
    }
    if (seven == nil || nine == nil || zero == nil) {
        raise "missing granted Unibus files";
    }
    take(nine, 9, "nine");
    take(nine, 9, "rx-live");
    # EN: The fixed tag7 record confirms that this guest reached init and loaded its module.
    # RU: Запись tag7 подтверждает, что этот гость дошёл до init и загрузил модуль.
    started := array[] of {byte 186, byte 133, byte 135, byte 111, byte 117, byte 116, byte 48};
    if (sys->write(seven, started, len started) != len started) {
        raise "Unibus startup record refused";
    }
    take(nine, 9, "started");
    take(zero, 0, "local-in");
    take(seven, 7, "zero");
    take(seven, 7, "one");
    take(seven, 7, "two");
    take(seven, 7, "new");
    # Fixed original S-record: tag0/local-out, never root0.
    local := array[] of {byte 178, byte 138, byte 128, byte 108, byte 111, byte 99, byte 97, byte 108, byte 45, byte 111, byte 117, byte 116};
    if (sys->write(zero, local, len local) != len local) {
        raise "Unibus write refused";
    }
    take(nine, 9, "guest-verified");
    sys->print("PASS real Dis guest file Unibus bidirectional records\n");
}
