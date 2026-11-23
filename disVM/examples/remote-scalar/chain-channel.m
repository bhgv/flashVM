# Copyright (c) 2026 bhgv
# EN: Ordinary import for a library that takes live integer channels.
# RU: Обычный импорт библиотеки, принимающей живые каналы int.
Chainchannel: module {
    exchange: fn(op: int, input: chan of int, ports: (chan of int, chan of int)): int;
};
