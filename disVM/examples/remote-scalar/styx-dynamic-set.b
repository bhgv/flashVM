# Copyright (c) 2026 bhgv
# EN: Exercise arbitrary scalar layouts through ordinary load in a separate VM.
# RU: Проверить произвольные скалярные раскладки через обычный load в отдельной VM.
implement Client;
include "sys.m";
include "dynamic-set.m";

Client: module { init: fn(); };
Staticonly: module { tally: fn(amount: int): int; };


# EN: Check zero to four cells, nested results, void, and continued use after an exception.
# RU: Проверить от нуля до четырёх ячеек, вложенные результаты, void и вызов после ошибки.
init()
{
    sys := load Sys Sys->PATH;
    target := load Dynamicset "/peer/mounted/dynamic.dis";
    if (sys->open("expect-legacy-dynamic-refusal", Sys->OREAD) != nil) {
        if (target != nil) {
            raise "dynamic export linked without compiled import types";
        }
        legacy := load Staticonly "/peer/mounted/dynamic.dis";
        if (legacy == nil || legacy->tally(7) != 7) {
            raise "legacy client could not load the fixed subset";
        }
        sys->print("PASS dynamic export requires compiled import types\n");
        return;
    }
    if (target == nil || target->status() != 0) {
        raise "dynamic empty argument list failed";
    }
    target->adjust(big -5);
    if (target->status() != -5) {
        raise "dynamic void result or shared state failed";
    }
    min := -big 9223372036854775807 - big 1;
    max := big 9223372036854775807;
    (large, total) := target->project((min, 3));
    if (large != min || total != -2) {
        raise "dynamic two-cell result failed";
    }
    amount: int;
    (total, (amount, large)) = target->fold(5, (max, 7));
    if (total != 10 || amount != 7 || large != max) {
        raise "dynamic nested result failed";
    }
    tail: big;
    (large, (total, amount), tail) = target->quad((max, 2), -3, min);
    if (large != max || total != 9 || amount != -3 || tail != min) {
        raise "dynamic four-cell result failed";
    }
    {
        target->fold(-1, (min, 0));
        raise "expected dynamic owner exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (target->tally(0) != 9 || target->status() != 9) {
        raise "dynamic route lost state after exception";
    }
    target->clear();
    if (target->tally(2) != 2 || target->status() != 2) {
        raise "dynamic empty call or fixed profile state failed";
    }
    sys->print("PASS mounted dynamic scalar call layouts\n");
}
