# Copyright (c) 2026 bhgv
# EN: Exercise copy-back from an original mounted Dis export using ordinary load.
# RU: Проверить возврат обновлений из исходного Dis через обычный load.
implement Client;
include "sys.m";
include "ref.m";

Client: module { init: fn(); };


# EN: Check alias identity, exception isolation and state across remote calls.
# RU: Проверить общий объект, обработку исключения и состояние между вызовами.
init()
{
    sys := load Sys Sys->PATH;
    owner := load Refexample "/peer/mounted/ref.dis";
    if (owner == nil) {
        raise "could not load original ref module";
    }
    min := -big 9223372036854775807 - big 1;
    max := big 9223372036854775807;
    cell := ref Cell(40, min);
    alias := cell;
    (count, wide) := owner->revise(cell);
    if (count != 1 || wide != min || alias.narrow != 41 || alias.wide != max) {
        raise "original ref result or copy-back was lost";
    }
    cell.narrow = 0;
    cell.wide = min;
    {
        owner->revise(cell);
        raise "expected remote ref exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (alias.narrow != 0 || alias.wide != min) {
        raise "failed original ref call modified caller";
    }
    {
        owner->revise(nil);
        raise "expected nil ref rejection";
    } exception {
        "invalid remote ref argument" =>
            ;
    }
    cell.narrow = 8;
    (count, wide) = owner->revise(cell);
    if (count != 3 || wide != min || alias.narrow != 9 || alias.wide != max) {
        raise "original ref owner lost state or copy-back after error";
    }
    sys->print("PASS mounted original ref Dis\n");
}
