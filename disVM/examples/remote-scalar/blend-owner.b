# Copyright (c) 2026 bhgv
# EN: Original Dis provider demonstrating argument order and nested return.
# RU: Исходная библиотека Dis для проверки порядка аргументов и вложенного результата.
implement Blend;
include "blend.m";

calls: int; # Persistent state of the original owner library.


# EN: Count admitted calls and return scaled fields, or raise for a zero seed.
# RU: Посчитать допущенные вызовы и вернуть вычисленные поля либо ошибку при seed=0.
blend(seed: int, pair: (int,big), delta: big): (int,(int,big))
{
    (narrow, wide) := pair;
    calls++;
    if (seed == 0) {
        raise "zero blend seed";
    }
    return (calls, (seed + narrow, wide + delta));
}
