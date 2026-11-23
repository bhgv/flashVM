# Copyright (c) 2026 bhgv
# EN: Load a FlashVM-backed module and invoke its script through normal Dis calls.
# RU: Загрузить модуль FlashVM и вызвать его скрипт штатными вызовами Dis.
implement Client;
include "sys.m";

Client: module {
    init: fn();
};

Flash: module {
    next: fn(n: int): int;
    reject: fn(n: int): int;
};


# EN: Use ordinary load and check two signed values from fresh RPC calls.
# RU: Использовать обычный load и проверить два знаковых значения отдельных вызовов.
init()
{
    sys := load Sys Sys->PATH;
    flash := load Flash "/rpc/mounted/flash.dis";
    if (flash == nil) {
        raise sys->sprint("cannot load FlashVM export: %r");
    }
    if (flash->next(41) != 42) {
        raise "wrong first result from FlashVM AVM1";
    }
    refused := 0;
    {
        flash->reject(10);
    } exception {
        "*" => refused = 1;
    }
    if (!refused || flash->next(-10) != -9) {
        raise "wrong result from FlashVM AVM1";
    }
    sys->print("PASS DisVM loaded FlashVM AVM1 through Styx RPC\n");
}
