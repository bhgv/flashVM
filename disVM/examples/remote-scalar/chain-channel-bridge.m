# Copyright (c) 2026 bhgv
# EN: Cross a second mounted library with live channels in both directions.
# RU: Передать живые каналы в обоих направлениях через второй маунт библиотеки.
Channelbridge: module
{
    roundtrip: fn(port: chan of int, value: int): (int, chan of int);
    waitvalue: fn(port, entered: chan of int): int;
    worker: fn(start: int): (chan of int, chan of int);
};
