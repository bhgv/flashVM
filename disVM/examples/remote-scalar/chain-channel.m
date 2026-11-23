# Copyright (c) 2026 bhgv
# EN: Ordinary import for a library that takes live integer channels.
# RU: Обычный импорт библиотеки, принимающей живые каналы int.
Chainchannel: module {
    Item: adt {
        number: int;
    };
    Payload: type (string, array of int, ref Item, ref Item);
    exchange: fn(op: int, input: chan of int, ports: (chan of int, chan of int)): int;
    typed: fn(input, output: chan of Payload, wide: chan of big): int;
    scalars: fn(tiny: chan of byte, fraction: chan of real, text: chan of string): int;
    channelresult: fn(input: chan of int, mode: int): chan of int;
};
