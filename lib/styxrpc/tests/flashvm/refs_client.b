# Copyright (c) 2026 bhgv
# EN: Load AS exports from Dis and check reference identity and failure rollback.
# RU: Загрузка экспортов AS из Dis с проверкой идентичности и отката при отказе.
implement Client;
include "sys.m";
include "refs.m";

Client: module { init: fn(); };


# EN: Pass the same cell twice, then observe a refused mutation and a new success.
# RU: Передать один объект дважды, затем проверить отклонённое изменение и новый успех.
init()
{
    sys := load Sys Sys->PATH;
    refs := load Refs "/rpc/mounted/flash-refs.dis";
    if (refs == nil) {
        raise sys->sprint("cannot load FlashVM ref adapter: %r");
    }
    Cell: import refs;
    bytes := array[] of {byte 7, byte 0};
    left := ref Cell("old", bytes);
    other := ref Cell("other", nil);
    if (refs->change("new", left, left, other) != "new" || left.text != "new" ||
        len left.bytes != 2 || left.bytes[0] != byte 7 || left.bytes[1] != byte 0 || other.text != "other") {
        raise "FlashVM alias or write-back failed";
    }
    denied := 0;
    {
        refs->reject("discard", left, left, other);
    } exception {
        "*" => denied = 1;
    }
    if (!denied || left.text != "new" || other.text != "other" ||
        refs->change("again", left, left, other) != "again" || left.text != "again") {
        raise "failed FlashVM call committed a ref update";
    }
    if (refs->change("nil", left, left, nil) != "nil" || left.text != "nil") {
        raise "FlashVM rejected an absent ref";
    }
    sys->print("PASS DisVM loaded FlashVM ref exports\n");
}
