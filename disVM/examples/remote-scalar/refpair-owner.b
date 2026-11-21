# Copyright (c) 2026 bhgv
# EN: Original Dis implementation that observes writes through an aliased argument.
# RU: Исходная реализация Dis, наблюдающая записи через другую ссылку на тот же объект.
implement Refpairexample;
include "sys.m";
include "ref.m";

calls: int; # Counter in the linked owner instance.


# EN: Mutate first, read through second, and mutate second before returning.
# RU: Изменить first, прочитать через second и изменить second до возврата.
exchange(first, second: ref Cell): (int,big)
{
    reject := first.narrow == 0;
    exercise_conflict := first.narrow == -7;
    calls++;
    first.narrow += 2;
    first.wide = big 9223372036854775807;
    wide := second.wide;
    narrow := second.narrow;
    second.narrow += 3;
    second.wide = -big 9223372036854775807 - big 1;
    if (exercise_conflict) {
        sys := load Sys Sys->PATH;
        set_marker(sys, "refpair-entered");
        wait_marker(sys, "refpair-edited");
    }
    if (reject) {
        raise "ref pair example error after writes";
    }
    return (calls * 100 + narrow, wide);
}

include "refpair-sync.b";
