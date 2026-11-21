# Copyright (c) 2026 bhgv
# EN: Test a mounted original Dis with shared and distinct ref arguments.
# RU: Проверить смонтированный исходный Dis с общими и разными аргументами ref.
implement Client;
include "sys.m";
include "ref.m";

Client: module { init: fn(); };


# EN: Verify identity, signed widths, exception isolation and later calls.
# RU: Проверить идентичность, знаковые диапазоны, исключение и последующие вызовы.
init()
{
    sys := load Sys Sys->PATH;
    owner := load Refpairexample "/peer/mounted/refpair.dis";
    if (owner == nil) {
        raise "could not load original ref pair module";
    }
    min := -big 9223372036854775807 - big 1;
    max := big 9223372036854775807;
    first := ref Cell(7, min);
    alias := first;
    (result, wide) := owner->exchange(first, alias);
    if (result != 109 || wide != max || alias.narrow != 12 || alias.wide != min) {
        raise "remote ref alias lost object identity";
    }
    # Equal initial fields still describe two distinct heap objects.
    second := ref Cell(first.narrow, first.wide);
    (result, wide) = owner->exchange(first, second);
    if (result != 212 || wide != min || first.narrow != 14 || first.wide != max ||
        second.narrow != 15 || second.wide != min) {
        raise "remote ref pair merged distinct objects";
    }
    first.narrow = 0;
    {
        owner->exchange(first, second);
        raise "expected remote ref pair exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (first.narrow != 0 || first.wide != max || second.narrow != 15 || second.wide != min) {
        raise "failed remote ref pair updated the client";
    }
    {
        owner->exchange(first, nil);
        raise "expected nil second ref rejection";
    } exception {
        "invalid remote ref argument" =>
            ;
    }
    (result, wide) = owner->exchange(second, second);
    if (result != 417 || wide != max || second.narrow != 20 || second.wide != min) {
        raise "ref pair owner lost its state or accepted nil";
    }
    first.narrow = -7;
    done := chan of int;
    spawn edit_second(sys, second, done);
    {
        (result, wide) = owner->exchange(first, second);
        raise "expected concurrent ref pair conflict";
    } exception {
        "remote ref changed during call" =>
            ;
    }
    if (<-done != 1 || result != 417 || wide != max || first.narrow != -7 ||
        first.wide != max || second.narrow != 901 || second.wide != min) {
        raise "conflicting ref pair applied partial changes";
    }
    sys->print("PASS mounted original ref pair Dis\n");
}



# EN: Wait for the owner to receive the snapshots, then change one caller object.
# RU: Дождаться получения снимков владельцем и изменить один объект клиента.
edit_second(sys: Sys, second: ref Cell, done: chan of int)
{
    wait_marker(sys, "refpair-entered");
    second.narrow = 901;
    set_marker(sys, "refpair-edited");
    done <-= 1;
}

include "refpair-sync.b";
