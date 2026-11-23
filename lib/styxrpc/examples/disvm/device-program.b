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
}
