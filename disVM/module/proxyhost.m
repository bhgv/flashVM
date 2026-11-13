# Copyright (c) 2026 bhgv
# EN: Owner-thread native exports and rooted call arguments; private hosted NC64 API.
# RU: Native exports и rooted arguments на owner thread; private hosted NC64 API.
Proxyhost: module
{
    PATH: con "$Proxyhost"; # Hosted builtin, not a network endpoint.
    Variable: con 0; # Mutable cell owner.
    Function: con 1; # Cell used as a callback's captured context.
    Read, Write, Invoke: con iota; # Local owner operations, not wire opcodes.
    OK: con 0; # Operation succeeded.
    Capacity: con 4; # Bounded root/request/retirement pressure.


    # EN/RU: The original mutable cell; proxy clients carry handles instead.
    # Исходная mutable cell; proxy clients передают handles вместо неё.
    Cell: adt
    {
        value: int; # Original owner storage, also usable as a captured scalar.
    };


    # EN/RU: Open one bounded session on this permanent owner; return native status.
    # Открыть bounded session текущего постоянного owner; вернуть native status.
    open: fn(objects, requests: int): int;


    # EN/RU: Return zero inside an active owner call; blocking nested RPC is refused.
    # Вернуть ноль внутри active owner call; blocking nested RPC запрещён.
    canwait: fn(): int;


    # EN/RU: Root an existing cell separately; return status, session, slot, generation.
    # Отдельно укоренить cell; вернуть status, session, slot, generation.
    publish: fn(cell: ref Cell, kind: int): (int, big, int, big);


    # EN: Admit/resolve one request on its owner, retaining actual result refs.
    # Only status OK authorizes execution; caller must finish even after a handler error.
    # RU: Admit/resolve запроса на owner, удержать actual result refs.
    # Только OK разрешает исполнение; caller обязан finish даже после handler error.
    begin: fn(id: big, operation: int, session: big, slot: int, generation: big,
              leftsession: big, left: int, leftgen: big, rightsession: big, right: int,
              rightgen: big, refs, value: int):
              (int, ref Cell, ref Cell, ref Cell);


    # EN/RU: Finish exact active id after execution; status only, no replay.
    # Завершить точный active id после execution; только status, без replay.
    finish: fn(id: big): int;


    # EN/RU: Remove one peer owner; actual rooting/retirement stays owner-local.
    # Снять peer owner; фактические roots/retirement остаются owner-local.
    drop: fn(session: big, slot: int, generation: big): int;


    # EN/RU: Close and collect after finish; Capacity preserves pending roots for retry.
    # Закрыть и собрать после finish; Capacity сохраняет pending roots для retry.
    close: fn(): int;
};
