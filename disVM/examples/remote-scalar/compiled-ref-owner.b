# Copyright (c) 2026 bhgv
# EN: Exercise shape-selected calls against ordinary Dis exports and shared data.
# RU: Проверить вызовы по схеме на обычных экспортах Dis с общими данными.
implement Compiledref;
include "sys.m";
include "compiled-ref.m";

calls: int; # Counts calls across the three linked views.


# EN: Update one object and return a single NC64 int cell.
# RU: Изменить один объект и вернуть одну ячейку NC64 int.
bump(value: ref Cell): int
{
    calls++;
    value.narrow++;
    return calls;
}


# EN: Mutate both refs, with loss of remote changes on failure or a local conflict.
# RU: Изменить обе ссылки, чтобы проверить отмену записи при отказе или конфликте.
revise(values: (ref Cell, ref Cell))
{
    calls++;
    (first, second) := values;
    previous := first.narrow;
    first.narrow++;
    second.narrow += 2;
    if (previous == -11) {
        raise "owner rejected change";
    }
    if (previous == -10) {
        sys := load Sys Sys->PATH;
        set_marker(sys, "refpair-entered");
        wait_marker(sys, "refpair-edited");
    }
}


# EN: Report whether the two refs designate the same object without changing it.
# RU: Сообщить, обозначают ли ссылки один объект, и не менять его.
inspect(values: (ref Cell, ref Cell)): (int,big)
{
    (first, second) := values;
    if (first == second) {
        return (-calls, first.wide);
    }
    return (calls, first.wide);
}

include "refpair-sync.b";
