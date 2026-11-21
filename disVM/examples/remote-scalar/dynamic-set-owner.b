# Copyright (c) 2026 bhgv
# EN: Original Dis export implementation with one counter shared by dynamic calls.
# RU: Исходная реализация Dis с общим счётчиком динамических вызовов.
implement Dynamicset;
include "dynamic-set.m";

total: int;


# EN: Return the accumulated total without arguments.
# RU: Вернуть накопленный итог без аргументов.
status(): int
{
    return total;
}


# EN: Update the shared total and return no value.
# RU: Изменить общий итог без возврата значения.
adjust(delta: big)
{
    total += int delta;
}


# EN: Add the narrow field and return the untouched full-width field and total.
# RU: Прибавить узкое поле и вернуть прежнее широкое поле и итог.
project(value: (big,int)): (big,int)
{
    (large, amount) := value;
    total += amount;
    return (large, total);
}


# EN: Verify ordinary exception propagation as well as a mixed nested result.
# RU: Проверить передачу исключения и вложенный результат смешанных типов.
fold(seed: int, value: (big,int)): (int,(int,big))
{
    if (seed < 0) {
        raise "negative dynamic seed";
    }
    (large, amount) := value;
    total += seed + amount;
    return (total, (amount, large));
}


# EN: Return four ordered cells and update the same total.
# RU: Вернуть четыре ячейки в заданном порядке и изменить тот же итог.
quad(value: (big,int), step: int, tail: big): (big,(int,int),big)
{
    (large, amount) := value;
    total += amount + step;
    return (large, (total, step), tail);
}


# EN: Reset the shared total with an empty input and output.
# RU: Обнулить общий итог без аргументов и результата.
clear()
{
    total = 0;
}


# EN: Update the same data through the existing fixed int profile.
# RU: Изменить те же данные через прежний профиль int.
tally(amount: int): int
{
    total += amount;
    return total;
}


# EN: Compute a fraction in the owning VM and return the original real alongside it.
# RU: Вычислить дробное значение в VM владельца и рядом вернуть исходное.
fractional(input: (byte,byte), value, shift: real): (real,(byte,byte),real)
{
    (first, second) := input;
    return (value * real int second + shift, (second, first), value);
}
