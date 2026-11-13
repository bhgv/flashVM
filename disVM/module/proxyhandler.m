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


    # EN: Execute on original owner and return an actual Cell as variable or
    # function capture. Borrowed arguments stay pinned through export publication.
    # RU: Исполнить на исходном owner; вернуть actual Cell как variable либо
    # function capture. Borrowed arguments pinned до export publication.
    result: fn(capture, left, right: ref Proxyhost->Cell, value, kind: int): ref Proxyhost->Cell;


    # EN: Execute with variable-only proxies to other owners. Each access shares
    # the original absolute deadline; no raw foreign Cell or automatic replay.
    # RU: Исполнить с variable-only proxies других owners. Все accesses делят
    # исходный absolute deadline; без raw foreign Cell и automatic replay.
    routed: fn(capture: ref Proxyhost->Cell, left, right: ref Proxyref->Ref,
               refs, value, deadline: int): int;
};
