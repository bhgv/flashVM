# Copyright (c) 2026 bhgv
# EN: Compiled interface for passing Dis-owned channels to FlashVM callbacks.
# RU: Компилированный интерфейс передачи каналов Dis обработчикам FlashVM.
Incoming: module {
    exchange: fn(ports: array of chan of int, messages: chan of string): int;
    relay: fn(port: chan of int): int;
    reject: fn(port: chan of int);
    finish: fn(port: chan of int);
    timeout: fn(port: chan of int): int;
};
