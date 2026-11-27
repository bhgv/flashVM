# Copyright (c) 2026 bhgv
# EN: Load a managed FlashVM export and verify a fragmented byte-array exchange.
# RU: Загрузить управляемый экспорт FlashVM и проверить обмен фрагментированным массивом.
implement Client;
include "sys.m";

Client: module { init: fn(); };

Flash: module {
    echo: fn(text: string, data: array of byte): (string, array of byte);
    number: fn(value: real, label: string): (real, string);
    wide: fn(words: array of string, values: array of big): (array of string, array of big);
    listcopy: fn(items: list of (string, big)): list of (string, big);
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
    (value, label) := flash->number(-12.5, "number");
    if (value != -12.5 || label != "number") {
        raise "wrong real from FlashVM";
    }
    words := array[] of {"9223372036854775807", "Привет 🌍"};
    numbers := array[] of {big "-9223372036854775808", big "-9007199254740992", big "-9007199254740991",
        big "9007199254740991", big "9007199254740992", big "9223372036854775807"};
    (strings, wide) := flash->wide(words, numbers);
    if (len strings != len words || len wide != len numbers || strings == words || wide == numbers) {
        raise "wide managed call did not return distinct value arrays";
    }
    for (i = 0; i < len numbers; i++) {
        if (wide[i] != numbers[i]) {
            raise "wide value changed across the two VM boundary";
        }
    }
    for (i = 0; i < len words; i++) {
        if (strings[i] != words[i]) {
            raise "ordinary string changed across the wide call";
        }
    }
    before := ("Привет", big "-9223372036854775808") :: ("end", big "9223372036854775807") :: nil;
    after := flash->listcopy(before);
    if (len after != 2 || before == after || flash->listcopy(nil) != nil) {
        raise "wrong FlashVM copied list";
    }
    (name, minimum) := hd after;
    (last, maximum) := hd tl after;
    if (name != "Привет" || last != "end" || minimum != big "-9223372036854775808" ||
        maximum != big "9223372036854775807") {
        raise "wrong FlashVM copied list heads";
    }
    sys->print("PASS DisVM loaded FlashVM managed values through Styx RPC\n");
}
