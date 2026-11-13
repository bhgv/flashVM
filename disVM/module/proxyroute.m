# Copyright (c) 2026 bhgv
# EN: Own one remote variable image behind a GC-tracked local variable-only actor.
# RU: Владеть remote variable image через GC-tracked local variable-only actor.
Proxyroute: module
{
    PATH: con "proxyroute.dis"; # Matched bytecode adapter; no serialized local channels.


    # EN/RU: Initialize immutable dependencies before sharing with access actors.
    # Инициализировать immutable dependencies до sharing с access actors.
    init: fn();


    # EN: Run on its own permanent Prog. Clone source before announcing an owned
    # Proxyref; timeout bounds each source exchange and idle lifetime. Last ref GC,
    # Close, idle or source failure retires this actor and its independent owner.
    # RU: Исполнить на своём permanent Prog. Clone source до передачи owned Proxyref;
    # timeout ограничивает exchange/idle. Last ref GC, Close, idle либо source
    # failure завершают actor и его independent owner.
    start: fn(source: ref Proxynet->Endpoint, image: Proxynet->Image, timeout: int,
              ready: chan of (int, ref Proxyref->Ref));
};
