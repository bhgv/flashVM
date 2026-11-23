# Copyright (c) 2026 bhgv
# EN: Load and call a virtual device module implemented by a standalone C process.
# RU: Загрузить и вызвать виртуальный модуль устройства из самостоятельного процесса C.
implement Client;
include "sys.m";

Client: module { init: fn(); };

Device: module
{
    Cell: adt { counter: int; };
    adjust: fn(text: string, samples: array of int, item: ref Cell): (string, int);
    sample: fn(value: int): int;
};


# EN: Require an ordinary Dis load and the C callback's returned value.
# RU: Проверить обычный load Dis и значение, возвращённое колбэком C.
init()
{
    sys := load Sys Sys->PATH;
    device := load Device "/device/mounted/device.dis";
    if (device == nil) {
        raise sys->sprint("cannot load C device: %r");
    }
    if (device->sample(41) != 42 || device->sample(-10) != -9) {
        raise "wrong C device result";
    }
    item := ref Device->Cell(37);
    (text, counter) := device->adjust("sensor", array[] of {5, 99}, item);
    if (text != "sensor" || counter != 42 || item.counter != 42) {
        raise "wrong typed C device result or missing ref writeback";
    }
    (text, counter) = device->adjust("unchanged", array[] of {0}, item);
    if (text != "unchanged" || counter != 42 || item.counter != 42) {
        raise "wrong typed reply without ref writeback";
    }
    path := "/device/mounted/device.dis/value";
    reader := sys->open(path, Sys->OREAD);
    writer := sys->open(path, Sys->OWRITE);
    data := array[1] of byte;
    if (reader == nil || writer == nil ||
        sys->read(reader, data, 1) != 1 || data[0] != byte 'A') {
        raise sys->sprint("cannot read device value: %r");
    }
    data[0] = byte 'Z';
    if (sys->write(writer, data, 1) != 1) {
        raise sys->sprint("cannot write device value: %r");
    }
    reader = sys->open(path, Sys->OREAD);
    if (reader == nil || sys->read(reader, data, 1) != 1 || data[0] != byte 'Z') {
        raise sys->sprint("device value did not update: %r");
    }
    sys->print("PASS Dis load of standalone C device\n");
}
