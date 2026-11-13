# Copyright (c) 2026 bhgv
# EN: User callback executes inside its serving owner's permanent Prog.
# RU: Пользовательский callback исполняется в постоянном Prog обслуживающего owner.
Proxyhandler: module
{
    # EN/RU: Initialize the original captured cell on the serving owner.
    # Инициализировать исходную captured cell на обслуживающем owner.
    start: fn(capture: ref Proxyhost->Cell);


    # EN/RU: Invoke with actual owner-backed refs; return a copied scalar or raise.
    # Выполнить с actual owner-backed refs; вернуть scalar copy либо raise.
    invoke: fn(capture, left, right: ref Proxyhost->Cell, value: int): int;
};
