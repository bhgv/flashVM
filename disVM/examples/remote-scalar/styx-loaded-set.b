# Copyright (c) 2026 bhgv
# EN: Call a mixed set of exports through one ordinary mounted module.
# RU: Вызвать разные экспорты одного смонтированного модуля через обычный load.
implement Client;
include "sys.m";
include "loaded-set.m";

Client: module { init: fn(); };
Partial: module { tally: fn(amount: int): int; };
Wrong: module { wide: fn(value: int): int; };


# EN: Verify that both imports reach the same state across all supported profiles.
# RU: Проверить общее состояние двух импортов во всех поддерживаемых профилях.
init()
{
    sys := load Sys Sys->PATH;
    path := "/peer/mounted/set.dis";
    part := load Partial path;
    if (sys->open("expect-set-refusal", Sys->OREAD) != nil) {
        if (part != nil) {
            raise "partially linked owner became callable";
        }
        sys->print("PASS loaded export set refused as a whole\n");
        return;
    }
    wrong := load Wrong path;
    owner := load Loadedset path;
    if (part == nil || wrong != nil || owner == nil) {
        raise "loaded set linked the wrong profile";
    }
    min := -big 9223372036854775807 - big 1;
    max := big 9223372036854775807;
    if (part->tally(10) != 10 || owner->wide(min) != min + big 11) {
        raise "int and big views lost shared state";
    }
    (count, value) := owner->pair((2, max));
    if (count != 13 || value != max) {
        raise "tuple view lost shared state";
    }
    cell := ref Cell(1, max);
    (count, value) = owner->revise(cell);
    if (count != 14 || cell.narrow != 15 || value != max) {
        raise "ref view lost shared state or copy-back";
    }
    narrow: int;
    (count, (narrow, value)) = owner->blend(3, (4, big 2), big -1);
    if (count != 21 || narrow != 4 || value != big 1) {
        raise "list view lost shared state or result";
    }
    (count, value) = owner->exchange(cell, cell);
    if (count != 22 || cell.narrow != 38 || value != max) {
        raise "ref pair view lost shared state or alias identity";
    }
    {
        owner->pair((-1, min));
        raise "expected loaded set owner error";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (part->tally(0) != 22) {
        raise "owner did not continue on the same instance";
    }
    sys->print("PASS mounted loaded export set shares state\n");
}
