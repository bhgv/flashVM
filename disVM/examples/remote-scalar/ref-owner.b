# Copyright (c) 2026 bhgv
# EN: Original Dis implementation with persistent module state and a mutable argument.
# RU: Исходная реализация Dis с состоянием модуля и изменяемым аргументом.
implement Refexample;
include "ref.m";

calls: int; # Mutable state in the loaded owner instance.


# EN: Update the passed cell and return the call number and prior wide field.
# RU: Обновить переданный объект и вернуть номер вызова и прежнее широкое поле.
revise(value: ref Cell): (int,big)
{
    calls++;
    (narrow, wide) := (value.narrow, value.wide);
    value.narrow = narrow + 1;
    value.wide = big 9223372036854775807;
    if (narrow == 0) {
        raise "ref owner example error";
    }
    return (calls, wide);
}
