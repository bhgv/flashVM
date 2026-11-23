# Copyright (c) 2026 bhgv
# EN: Compiled argument shapes for a FlashVM caller's reverse UniBus.
# RU: Компилированные формы аргументов встречной шины вызывающего FlashVM.
Arguments: module {
    exchange: fn(ports: array of chan of int, messages: chan of string): int;
    reject: fn(port: chan of int);
    result: fn(port: chan of int): chan of int;
    relay: fn(port: chan of int): int;
    nested: fn(batch: chan of array of chan of int, direct: chan of int, fail: int): int;
    wait: fn(port: chan of int): int;
};
