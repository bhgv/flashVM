# Copyright (c) 2026 bhgv
# EN: Ordinary import for a library that takes live integer channels.
# RU: Обычный импорт библиотеки, принимающей живые каналы int.
Chainchannel: module {
    Item: adt {
        number: int;
    };
    Payload: type (string, array of int, ref Item, ref Item);
    Relayed: type (string, array of chan of int, chan of int);
    exchange: fn(op: int, input: chan of int, ports: (chan of int, chan of int)): int;
    typed: fn(input, output: chan of Payload, wide: chan of big): int;
    scalars: fn(tiny: chan of byte, fraction: chan of real, text: chan of string): int;
    channelresult: fn(input: chan of int, mode: int): chan of int;
    nested: fn(input, output: chan of Relayed, known: chan of int): chan of int;
    choose: fn(ports: array of chan of int): (int, int, array of chan of int);
};
