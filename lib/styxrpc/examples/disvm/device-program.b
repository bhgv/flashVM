# Copyright (c) 2026 bhgv
# EN: Check the C caller's VFS, shared variable and write-only numbered descriptor.
# RU: Проверка VFS, общей переменной и дескриптора записи C-клиента.
implement Deviceprogram;
include "sys.m";
include "launchcontext.m";

Deviceprogram: module { init: fn(); };


# EN: Read granted files on the caller and publish one variable and one FD update.
# RU: Прочитать выданные файлы клиента и записать переменную и дескриптор.
init()
{
    sys := load Sys Sys->PATH;
    context := load Launchcontext Launchcontext->PATH;
    if (string context->arguments() == "park") {
        parked := sys->open("/.vfs/parked", Sys->OWRITE);
        if (parked == nil || sys->write(parked, array[] of {byte 'P'}, 1) != 1) {
            raise "missing cancellation checkpoint";
        }
        remaining := big 1 << 50;
        while (remaining != big 0) {
            remaining--;
        }
        raise "cancelled process reached its regular return";
    }
    data := array[4] of byte;
    pid := sys->open("/.prog/.self/.pid", Sys->OREAD);
    marker := sys->open("/.vfs/marker", Sys->OREAD);
    count := sys->open("/.prog/.self/.var/count", Sys->OREAD);
    update := sys->open("/.prog/.self/.var/count", Sys->OWRITE);
    output := sys->fildes(7);
    input := sys->fildes(0);
    stdout := sys->fildes(1);
    if (input == nil || stdout == nil || sys->read(input, data, 1) != 1 || data[0] != byte 'I' ||
        sys->write(stdout, array[] of {byte 'O'}, 1) != 1) {
        raise sys->sprint("wrong standard descriptors: %r");
    }
    if (pid == nil || marker == nil || count == nil || output == nil || update == nil ||
        sys->read(pid, data, 3) != 3 || string data[0:3] != "42\n" ||
        sys->read(marker, data, 2) != 2 || string data[0:2] != "C\n" ||
        sys->read(count, data, 4) != 4 ||
        data[0] != byte 0 || data[1] != byte 0 || data[2] != byte 0 || data[3] != byte 41) {
        raise sys->sprint("wrong reverse launch context: %r");
    }
    data[3] = byte 42;
    if (sys->write(update, data, 4) != 4 ||
        sys->open("/.prog/.self/.fd/7", Sys->OREAD) != nil ||
        sys->write(output, array[] of {byte 'D'}, 1) != 1) {
        raise sys->sprint("wrong reverse variable or descriptor: %r");
    }
    collections(sys);
    hybridtable(sys);

}

# EN: Write one command through its own opened mutation file.
# RU: Передать одну команду через её отдельный открытый файл.
put(sys: Sys, path: string, data: array of byte): int
{
    fd := sys->open(path, Sys->OWRITE);
    return fd != nil && sys->write(fd, data, len data) == len data;
}

# EN: Mutate the vector and keyed map, then select each through an exact .idx fid.
# RU: Изменить массив и таблицу ключей, затем выбрать значение точным fid .idx.
collections(sys: Sys)
{
    vector := "/.prog/.self/.var/vector/";
    keyed := "/.prog/.self/.var/keyed/";
    if (!put(sys, vector + ".ins", array[] of {byte 0, byte 0, byte 0, byte 1,
            byte 0, byte 0, byte 0, byte 12}) ||
        !put(sys, vector + ".push", array[] of {byte 0, byte 0, byte 0, byte 99}) ||
        !put(sys, keyed + ".add", array[] of {byte 0, byte 0, byte 0, byte 9,
            byte 0, byte 0, byte 0, byte 77}) ||
        put(sys, keyed + ".add", array[] of {byte 0, byte 0, byte 0, byte 9,
            byte 0, byte 0, byte 0, byte 77})) {
        raise sys->sprint("collection mutation failed: %r");
    }
    pop := sys->open(vector + ".pop", Sys->OREAD);
    data := array[4] of byte;
    if (pop == nil || sys->read(pop, data, 4) != 4 || data[3] != byte 99 ||
        sys->seek(pop, big 0, Sys->SEEKSTART) != big 0 ||
        sys->read(pop, data, 4) != 4 || data[3] != byte 99 ||
        sys->read(pop, data, 4) != 0) {
        raise "wrong or repeated collection pop";
    }
    selector := sys->open(vector + ".idx", Sys->OWRITE);
    index := array[] of {byte 0, byte 0, byte 0, byte 2};
    if (selector == nil || sys->write(selector, index, 4) != 4 ||
        sys->open(vector + ".idx", Sys->OWRITE) != nil) {
        raise "one collection selector was not held exclusively";
    }
    member := sys->open(vector + ".val", Sys->ORDWR);
    if (member == nil || sys->read(member, data, 4) != 4 || data[3] != byte 22 ||
        sys->seek(member, big 0, Sys->SEEKSTART) != big 0) {
        raise "wrong selected vector value";
    }
    data[3] = byte 23;
    if (sys->write(member, data, 4) != 4) {
        raise "could not update selected vector value";
    }
    selector = nil;
    for (tries := 0; tries < 100; tries++) {
        if (sys->open(vector + ".val", Sys->OREAD) == nil) {
            break;
        }
        sys->sleep(5);
    }
    if (tries == 100 || sys->seek(member, big 0, Sys->SEEKSTART) != big 0 ||
        sys->read(member, data, 4) >= 0) {
        raise "closing .idx did not retire its exact value fid";
    }
    dtype := sys->open(keyed + ".type", Sys->OREAD);
    length := sys->open(vector + ".len", Sys->OREAD);
    metadata := array[16] of byte;
    if (dtype == nil || length == nil || sys->read(dtype, metadata, len metadata) != 13 ||
        string metadata[0:13] != "table of int\n" || sys->read(length, metadata, len metadata) != 2 ||
        string metadata[0:2] != "3\n") {
        raise "wrong collection metadata";
    }
    selector = sys->open(keyed + ".idx", Sys->OWRITE);
    index[3] = byte 9;
    if (selector == nil || sys->write(selector, index, 4) != 4) {
        raise "could not select newly added key";
    }
    member = sys->open(keyed + ".val", Sys->OREAD);
    if (member == nil || sys->read(member, data, 4) != 4 || data[3] != byte 77) {
        raise "wrong keyed collection value";
    }
}

# EN: Check and close one precise typed selector while the old value fid still exists.
# RU: Проверить и закрыть типизированный селектор, оставив старый fid значения открытым.
hybridcell(sys: Sys, path: string, key: array of byte, expected, replacement: int)
{
    selector := sys->open(path + ".idx", Sys->OWRITE);
    if (selector == nil || sys->write(selector, key, len key) != len key) {
        raise sys->sprint("hybrid selector: %r");
    }
    member := sys->open(path + ".val", Sys->ORDWR);
    data := array[8] of byte;
    if (member == nil || sys->read(member, data, 3) != 3 ||
        sys->read(member, data[3:], 5) != 5 || data[7] != byte expected) {
        raise "wrong selected hybrid table value";
    }
    if (replacement >= 0) {
        data[7] = byte replacement;
        if (sys->seek(member, big 0, Sys->SEEKSTART) != big 0 || sys->write(member, data, 8) != 8) {
            raise "hybrid table update denied";
        }
    }
    selector = nil;
    for (tries := 0; tries < 100; tries++) {
        if (sys->open(path + ".val", Sys->OREAD) == nil) {
            break;
        }
        sys->sleep(5);
    }
    if (tries == 100 || sys->seek(member, big 0, Sys->SEEKSTART) != big 0 || sys->read(member, data, 8) >= 0) {
        raise "hybrid table value fid survived selector clunk";
    }
}


# EN: Exercise both halves of a hybrid table through the stock Limbo file format.
# RU: Проверить обе части гибридной таблицы в штатном файловом формате Limbo.
hybridtable(sys: Sys)
{
    path := "/.prog/.self/.var/hybrid/";
    push := array[] of {byte 0, byte 0, byte 0, byte 0, byte 0, byte 0, byte 0, byte 99};
    insert := array[16] of { * => byte 0 };
    insert[7] = byte 1;
    insert[15] = byte 15;
    add := array[10] of { * => byte 0 };
    add[0] = byte 1;
    add[1] = byte 'x';
    add[9] = byte 77;
    if (!put(sys, path + ".ins", insert) || !put(sys, path + ".push", push) ||
        !put(sys, path + ".add", add) || put(sys, path + ".add", add)) {
        raise "wrong hybrid table mutation result";
    }
    pop := sys->open(path + ".pop", Sys->OREAD);
    data := array[24] of byte;
    if (pop == nil || sys->read(pop, data, 2) != 2 || sys->read(pop, data[2:], 6) != 6 ||
        data[7] != byte 99 || sys->seek(pop, big 0, Sys->SEEKSTART) != big 0 ||
        sys->read(pop, data, 8) != 8 || data[7] != byte 99) {
        raise "hybrid table pop was not cached";
    }
    count := sys->open(path + ".count", Sys->OREAD);
    length := sys->open(path + ".len", Sys->OREAD);
    typefile := sys->open(path + ".type", Sys->OREAD);
    if (count == nil || length == nil || typefile == nil ||
        sys->read(count, data, 24) != 2 || string data[0:2] != "6\n" ||
        sys->read(length, data, 24) != 2 || string data[0:2] != "3\n" ||
        sys->read(typefile, data, 24) != 13 || string data[0:13] != "table of big\n") {
        raise "wrong hybrid table metadata";
    }
    numeric := array[9] of { * => byte 0 };
    relative := array[9] of { * => byte 255 };
    relative[0] = byte 0;
    numeric[8] = byte 1;
    hybridcell(sys, path, numeric, 15, 17);
    hybridcell(sys, path, array[] of {byte 1, byte '1'}, 90, 91);
    hybridcell(sys, path, relative, 20, -1);
    numeric[8] = byte 5;
    hybridcell(sys, path, numeric, 50, -1);
}
