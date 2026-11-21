# Copyright (c) 2026 bhgv
# EN: Original Dis implementation of the mounted full-width example.
# RU: Исходная реализация Dis для примера полноразрядного вызова.
implement WideSet;
include "wide.m";

calls: big; # State retained by the owner's linked instance.


# EN: Echo the signed input and expose the instance count for the test.
# RU: Вернуть знаковый аргумент и выдать тесту счётчик экземпляра.
echo(value: big): big
{
    calls++;
    if (value == big 0) {
        raise "wide owner example error";
    }
    if (value == big 1) {
        return calls;
    }
    return value;
}



# EN: Update the shared counter from the narrow export.
# RU: Обновить общий счётчик из узкого экспорта.
count(delta: int): int
{
    calls += big delta;
    return int calls;
}
