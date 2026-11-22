# Copyright (c) 2026 bhgv
# EN: A mounted interface using the hosted hybrid table type.
# RU: Смонтированный интерфейс с нативным типом гибридной таблицы.
include "bigtable.m";

Hybridcall: module
{
    State: adt
    {
        source: ref Bigtable->Handle; # Alias of an argument / Алиас аргумента.
        stamp: int; # Ordinary field copied back with the tables / Поле в общем ответе.
    };

    apply: fn(mode: int, tab: ref Bigtable->Handle, peers: array of ref Bigtable->Handle,
        state: ref State): (ref Bigtable->Handle, ref Bigtable->Handle);
    empty: fn(): ref Bigtable->Handle;
};
