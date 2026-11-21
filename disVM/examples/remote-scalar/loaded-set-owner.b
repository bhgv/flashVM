# Copyright (c) 2026 bhgv
# EN: Original Dis implementation sharing state across six typed exports.
# RU: Исходная реализация Dis с общим состоянием шести экспортов.
implement Loadedset;
include "loaded-set.m";

total: int; # One state cell for all exports in the instance.


# EN: Accumulate amount and return the shared total.
# RU: Прибавить amount и вернуть общий счётчик.
tally(amount: int): int
{
    total += amount;
    return total;
}


# EN: Advance the shared counter and add it to the full-width value.
# RU: Увеличить общий счётчик и прибавить его к полноразрядному значению.
wide(value: big): big
{
    total++;
    return value + big total;
}


# EN: Add a nonnegative tuple field and return the shared total with the big field.
# RU: Прибавить неотрицательное поле кортежа и вернуть счётчик вместе с big.
pair(value: (int,big)): (int,big)
{
    (amount, widevalue) := value;
    if (amount < 0) {
        raise "negative loaded set amount";
    }
    total += amount;
    return (total, widevalue);
}


# EN: Copy the new total through value and return it with the unchanged big field.
# RU: Записать новый счётчик через value и вернуть его вместе с полем big.
revise(value: ref Cell): (int,big)
{
    total++;
    value.narrow += total;
    return (total, value.wide);
}


# EN: Accumulate both narrow arguments and return a nested tuple.
# RU: Прибавить оба узких аргумента и вернуть вложенный кортеж.
blend(seed: int, value: (int,big), delta: big): (int,(int,big))
{
    (narrow, widevalue) := value;
    total += seed + narrow;
    return (total, (narrow, widevalue + delta));
}


# EN: Update both references in order, with the same state visible through either.
# RU: Обновить обе ссылки по порядку, сохраняя общую идентичность объекта.
exchange(first, second: ref Cell): (int,big)
{
    total++;
    first.narrow += total;
    second.narrow++;
    return (total, second.wide);
}
