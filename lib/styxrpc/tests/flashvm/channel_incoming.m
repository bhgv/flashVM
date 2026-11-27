# Copyright (c) 2026 bhgv
# EN: Compiled interface for passing Dis-owned channels to FlashVM callbacks.
# RU: Компилированный интерфейс передачи каналов Dis обработчикам FlashVM.
Incoming: module {
    Box: adt {
        value: big; # Updated through a positional graph delta.
    };
    exchange: fn(ports: array of chan of int, messages: chan of string): int;
    relay: fn(port: chan of int): int;
    reject: fn(port: chan of int);
    finish: fn(port: chan of int);
    nested: fn(batch: chan of array of chan of int): int;
    make: fn(box: ref Box, port: chan of int): chan of int;
    sync: fn(port: chan of int): chan of chan of int;
    badseed: fn(port: chan of int): chan of int;
};
