# Copyright (c) 2026 bhgv
# EN: Actual portable console guest uses ordinary Sys FDs for granted Unibus records, no display or host builtin.
# RU: Настоящий переносимый консольный guest использует обычные Sys FD для записей Unibus, без display/host builtin.
implement Check;
include "sys.m";
include "vfsnative.m";
include "runexample.m";
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


# EN/RU: Read the client's environment through normal file operations.
# Прочитать окружение клиента обычными файловыми операциями.
clientenv(path: string): string
{
    file := sys->open(path, Sys->OREAD);
    bytes := array[5] of byte;
    if (file == nil || sys->read(file, bytes, len bytes) != len bytes) {
        return "";
    }
    return string bytes;
}


# EN/RU: Read the current client value and optionally publish one replacement.
# Прочитать значение клиента и при необходимости записать замену.
counter(expected, replacement, update: int): int
{
    path := "/.prog/.self/.var/counter";
    reader := sys->open(path, Sys->OREAD);
    data := array[4] of byte;
    if (reader == nil || sys->read(reader, data, 4) != 4) {
        return 0;
    }
    value := int data[0];
    if (value >= 128) {
        value -= 256;
    }
    for (i := 1; i < 4; i++) {
        value = value*256 + int data[i];
    }
    if (value != expected) {
        return 0;
    }
    if (!update) {
        return 1;
    }
    writer := sys->open(path, Sys->OWRITE);
    for (i = 0; i < 4; i++) {
        data[i] = byte (replacement >> (24 - 8*i));
    }
    return writer != nil && sys->write(writer, data, 4) == 4;
}


# EN/RU: Check the mounted array protocol with ordinary Sys file calls.
# Проверить протокол смонтированного массива обычными файловыми вызовами Sys.
checkarray()
{
    root := "/.prog/.self/.var/items/";
    push := sys->open(root + ".push", Sys->OWRITE);
    ins := sys->open(root + ".ins", Sys->OWRITE);
    data := array[] of {byte 0, byte 0, byte 0, byte 44};
    insertion := array[] of {byte 0, byte 0, byte 0, byte 1, byte 0, byte 0, byte 0, byte 77};
    if (push == nil || ins == nil || sys->write(push, data, 4) != 4 || sys->write(ins, insertion, 8) != 8) {
        raise "mounted array insertion failed";
    }
    pop := sys->open(root + ".pop", Sys->OREAD);
    if (pop == nil || sys->read(pop, data, 4) != 4 || int data[3] != 44) {
        raise "mounted array pop failed";
    }
    length := sys->open(root + ".len", Sys->OREAD);
    if (length == nil || sys->read(length, data, 4) != 2 || string data[0:2] != "3\n") {
        raise "mounted array length mismatch";
    }
    idx := sys->open(root + ".idx", Sys->OWRITE);
    index := array[] of {byte 0, byte 0, byte 0, byte 1};
    if (idx == nil || sys->write(idx, index, 4) != 4) {
        raise "mounted array selection failed";
    }
    val := sys->open(root + ".val", Sys->ORDWR);
    if (val == nil || sys->read(val, data, 4) != 4 || int data[3] != 77 ||
        sys->open(root + ".idx", Sys->OWRITE) != nil || sys->seek(val, big 0, Sys->SEEKSTART) != big 0) {
        raise "mounted array element mismatch";
    }
    data[3] = byte 99;
    if (sys->write(val, data, 4) != 4 || sys->seek(val, big 0, Sys->SEEKSTART) != big 0 ||
        sys->read(val, data, 4) != 4 || int data[3] != 99) {
        raise "mounted array update mismatch";
    }
    idx = nil;
    sys->print("PASS process indexed array over Styx\n");
}


# EN/RU: Verify keyed creation and value update through the mounted client table.
# Проверить создание записи и обновление значения в смонтированной таблице.
checktable()
{
    root := "/.prog/.self/.var/lookup/";
    key := array[] of {byte 255, byte 255, byte 255, byte 255};
    add := sys->open(root + ".add", Sys->OWRITE);
    pair := array[] of {byte 255, byte 255, byte 255, byte 255, byte 0, byte 0, byte 0, byte 45};
    if (add == nil || sys->write(add, pair, 8) != 8) {
        raise "mounted table add failed";
    }
    duplicate := sys->open(root + ".add", Sys->OWRITE);
    pair[7] = byte 11;
    if (duplicate == nil || sys->write(duplicate, pair, 8) != -1) {
        raise "duplicate table key was admitted";
    }
    length := sys->open(root + ".len", Sys->OREAD);
    data := array[4] of byte;
    if (length == nil || sys->read(length, data, 4) != 2 || string data[0:2] != "1\n") {
        raise "mounted table length mismatch";
    }
    idx := sys->open(root + ".idx", Sys->OWRITE);
    if (idx == nil || sys->write(idx, key, 4) != 4) {
        raise "mounted table key selection failed";
    }
    val := sys->open(root + ".val", Sys->ORDWR);
    if (val == nil || sys->read(val, data, 4) != 4 || int data[3] != 45 ||
        sys->seek(val, big 0, Sys->SEEKSTART) != big 0) {
        raise "mounted table value mismatch";
    }
    data[3] = byte 99;
    if (sys->write(val, data, 4) != 4 || sys->seek(val, big 0, Sys->SEEKSTART) != big 0 ||
        sys->read(val, data, 4) != 4 || int data[3] != 99) {
        raise "mounted table update mismatch";
    }
    idx = nil;
    sys->print("PASS process indexed table over Styx\n");
}


# EN/RU: Read the child's inherited aliases and confirm the parent's cursor continues.
# Прочитать выданный дочернему процессу FD с той же позиции, что и у родителя.
readchild(done: chan of string, hidestderr: int, expected: string)
{
    inherited := sys->fildes(3);
    part := array[3] of byte;
    if (inherited == nil || (sys->fildes(2) == nil) != hidestderr ||
        (hidestderr && (clientenv("/.prog/.self/.vfs/env/CLIENT") != expected || !counter(-2, 65539, 1))) || sys->read(inherited, part, 3) != 3) {
        done <-= "missing";
        return;
    }
    done <-= string part;
}


# EN/RU: Use inherited numbers without opening their Styx names; check one shared cursor.
# Использовать переданные номера без открытия имён Styx и проверить общую позицию.
checknumbered()
{
    first := sys->fildes(3);
    if (first == nil) {
        return;
    }
    second := sys->fildes(3);
    input := sys->fildes(0);
    output := sys->fildes(1);
    err := sys->fildes(2);
    if (second == nil || input == nil || output == nil || first.fd != 3 ||
        second.fd != 3 || input.fd != 0 || output.fd != 1 || (err != nil && err.fd != 2) ||
        sys->fildes(4) != nil || sys->fildes(32) != nil) {
        raise "incorrect numbered Sys FD";
    }
    part := array[3] of byte;
    if (sys->read(first, part, 3) != 3 || string part != "abc") {
        raise "numbered FD aliases lost shared cursor";
    }
    done := chan of string;
    environment := clientenv("/.prog/.self/.vfs/env/CLIENT");
    if (err == nil) {
        partial := sys->open("/.prog/.self/.var/counter", Sys->OWRITE);
        if (partial == nil || sys->write(partial, array[1] of byte, 1) != -1 ||
            !counter(1, -2, 1)) {
            raise "invalid partial or complete client variable write";
        }
    }
    spawn readchild(done, err == nil, environment);
    if (<-done != "def") {
        raise "child did not inherit numbered FD cursor";
    }
    first = nil;
    text := array of byte "!";
    if (sys->write(second, text, 1) != 1 || sys->write(input, text, 1) != -1 ||
        sys->read(output, part, 3) != -1) {
        raise "numbered FD mode or lifetime mismatch";
    }
    if (sys->read(input, part, 3) != 3 || string part != "xyz" ||
        (err != nil && sys->fprint(err, "remote stderr\n") != 14) || sys->print("remote stdout\n") != 14) {
        raise "numbered stdio did not use its launch file";
    }
    if (err == nil) {
        if (!counter(65539, 0, 0)) {
            raise "child variable write unavailable";
        }
        if ((environment != "child" && environment != "base!") ||
            sys->open("/env/CLIENT", Sys->OREAD) != nil || sys->open("/.vfs/env/CLIENT", Sys->OWRITE) != nil) {
            raise "process namespace grant not isolated";
        }
        pid := sys->open("/.prog/.self/.pid", Sys->OREAD);
        id := array[3] of byte;
        if (pid == nil || sys->read(pid, id, 3) != 3 || string id != "42\n" ||
            sys->open("/.prog/.self/.pid", Sys->OWRITE) != nil ||
            clientenv("/.vfs/env/CLIENT") != environment) {
            raise "incorrect private process alias";
        }
        checkarray();
        checktable();
        sys->print("PASS process context hides fd 2\n");
        sys->print("PASS process VFS %s\n", environment);
    }
    ordinary := sys->open("/nine", Sys->OREAD);
    if (ordinary == nil || ordinary.fd == 3) {
        raise "ordinary Sys open reused an inherited number";
    }
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
    (routed, _) := sys->stat("/routed.dis");
    if (routed == 0) {
        pending := sys->open("/reverse-exit", Sys->OREAD);
        if (pending == nil || sys->read(pending, array[1] of byte, 1) != -1) {
            raise "empty reverse completion should be pending";
        }
        vfs := load Vfsnative Vfsnative->PATH;
        if (vfs == nil || vfs->run("/missing.dis") != 0) {
            raise "VFS command submission failed";
        }
        if (vfs->runargs("/payload.dis", array[65537] of byte) == 1 ||
            vfs->runargs("/payload.dis", array[4] of byte) == 1) {
            raise "oversized launch payload accepted";
        }
        payload := array[] of {byte 'x', byte 0, byte 'y'};
        if (vfs->runargs("/payload.dis", payload) != 1) {
            raise "binary callback refused";
        }
        example := load Runexample Runexample->PATH;
        if (example == nil || example->execute("/routed.dis", "/routed.dis", "/run-result", payload) != 0) {
            raise "runnable VFS launch example failed";
        }
        if (vfs->run("/routed.dis") == 1) {
            raise "owner admitted a repeated request";
        }
        if (sys->open("/run-result", Sys->OWRITE) != nil) {
            raise "writable owner completion";
        }
        completion := sys->open("/run-result", Sys->OREAD);
        status := array[1] of byte;
        if (completion == nil || sys->read(completion, status, 1) != 1 ||
            status[0] != byte '0' || sys->read(completion, status, 1) != 1 ||
            status[0] != byte '\n' || sys->read(completion, status, 1) != 0) {
            raise "owner command did not finish over the live mount";
        }
        sys->print("PASS path-only Styx command submission\n");
    }
    checkheld();
    checknumbered();
    seven := sys->open("/seven", Sys->ORDWR);
    nine := sys->open("/nine", Sys->OREAD);
    zero := sys->open("/zero", Sys->ORDWR);
    if (sys->open("/.unibus", Sys->OREAD) != nil || sys->open("/records", Sys->OREAD) != nil || sys->open("/launch", Sys->OREAD) != nil ||
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
    if (routed == 0) {
        outcome := sys->open("/launch-exit", Sys->OREAD);
        if (outcome == nil) {
            raise "missing Unibus command completion file";
        }
        take(outcome, 17, "0\n");
    }
    if (sys->open("/reverse-exit", Sys->OWRITE) != nil) {
        raise "caller completion inbox permits guest writes";
    }
    reverse := sys->open("/reverse-exit", Sys->OREAD);
    if (reverse == nil) {
        raise "missing reverse Styx completion";
    }
    take(reverse, 17, "0\n");
    if (sys->read(reverse, array[1] of byte, 1) != 0) {
        raise "reverse completion did not reach EOF";
    }
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
