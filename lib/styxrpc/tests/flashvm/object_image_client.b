# Copyright (c) 2026 bhgv
# EN: Load one live object with mixed callbacks and shared variable files.
# RU: Загрузить живой объект со смешанными колбэками и общими файлами переменных.
implement Client;
include "sys.m";

Client: module { init: fn(); };
Image: module {
    add: fn(n: int): int;
    label: fn(s: string): (real, string);
    port: fn(n: int): chan of int;
};


# EN: Call scalar, managed and channel entries on one mounted object and update its cell.
# RU: Вызвать скалярный, управляемый и канальный входы одного объекта и изменить его ячейку.
init()
{
    sys := load Sys Sys->PATH;
    path := "/rpc/mounted/flash-object.dis";
    image := load Image path;
    if (image == nil) {
        raise sys->sprint("cannot load Flash object: %r");
    }
    if (image->add(2) != 42) {
        raise "scalar method lost its receiver";
    }
    count := sys->open(path + "/.var/count", Sys->ORDWR);
    if (count == nil) {
        raise sys->sprint("cannot open .var/count: %r");
    }
    data := array[] of {byte 0, byte 0, byte 0, byte 51};
    if (sys->write(count, data, len data) != len data || image->add(1) != 52) {
        raise "variable update did not reach the same object";
    }
    (number, text) := image->label("a\0б");
    if (number != 52.0 || text != "a\0б") {
        raise "managed method did not use the same state";
    }
    live := image->port(3);
    if (live == nil || <-live != 55) {
        raise "module channel result did not use the same receiver";
    }
    limit := sys->open(path + "/.var/limit", Sys->OWRITE);
    if (limit != nil) {
        raise "read-only variable opened for write";
    }
    reader := sys->open(path + "/.var/count", Sys->OREAD);
    if (reader == nil || sys->read(reader, data, len data) != len data || int data[3] != 52) {
        raise "method result was not visible to a new variable fid";
    }
    sys->print("PASS DisVM loaded FlashVM object image\n");
}
