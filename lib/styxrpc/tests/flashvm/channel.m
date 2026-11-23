# Copyright (c) 2026 bhgv
# EN: Compiled module interface for the native UniBus bridge.
# RU: Компилированный интерфейс моста нативного UniBus.
Channels: module {
    ports: fn(value: int): (chan of int, chan of int, chan of string);
    nested: fn(value: int): chan of array of chan of int;
    empty: fn(): chan of int;
};
