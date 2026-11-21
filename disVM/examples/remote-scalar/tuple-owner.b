# Copyright (c) 2026 bhgv
# EN: Stateful original Dis tuple implementation.
# RU: Реализация исходного кортежа Dis с состоянием.
implement Tuple;
include "tuple.m";

calls: int; # Owner instance state.


# EN: Return the call count with the original signed wide member.
# RU: Вернуть счётчик вызовов и исходное знаковое поле big.
transform(value: (int,big)): (int,big)
{
    (narrow, wide) := value;
    calls++;
    if (narrow == 0) {
        raise "tuple owner example error";
    }
    return (calls, wide);
}
