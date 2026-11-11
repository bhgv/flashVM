# Copyright (c) 2026 bhgv
# EN: Actual bytecode published as native leaf code while other residents execute it.
# RU: Настоящий байткод, публикуемый как native leaf-код при работе других потоков.
implement JitProvider;
include "provider.m";


# EN: Compute value*3+7 without shared mutable module data.
# RU: Вычислить value*3+7 без общих изменяемых данных модуля.
run(value: int): int
{
    return value * 3 + 7;
}
