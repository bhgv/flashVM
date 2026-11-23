# Copyright (c) 2026 bhgv
# EN: Load a managed FlashVM export and verify a fragmented byte-array exchange.
# RU: Загрузить управляемый экспорт FlashVM и проверить обмен фрагментированным массивом.
implement Client;
include "sys.m";

Client: module { init: fn(); };

Flash: module {
    echo: fn(text: string, data: array of byte): (string, array of byte);
};


# EN: Check counted Unicode, byte values and the nested tuple on an ordinary typed load.
# RU: Проверить Unicode, значения байтов и вложенный кортеж через штатный load.
init()
{
    sys := load Sys Sys->PATH;
    flash := load Flash "/rpc/mounted/flash-values.dis";
    if (flash == nil) {
        raise sys->sprint("cannot load managed FlashVM export: %r");
    }
    data := array[600] of byte;
    for (i := 0; i < len data; i++) {
        data[i] = byte (i % 256);
    }
    (text, copy) := flash->echo("Привет 🌍", data);
    if (text != "Привет 🌍" || len copy != 600 || copy == data) {
        raise "wrong managed FlashVM result";
    }
    for (i = 0; i < len copy; i++) {
        if (copy[i] != data[i]) {
            raise "wrong managed byte after fragmented transfer";
        }
    }
    (text, copy) = flash->echo(nil, nil);
    if (text != nil || copy != nil) {
        raise "nil managed FlashVM values changed representation";
    }
    sys->print("PASS DisVM loaded FlashVM managed values through Styx RPC\n");
}
