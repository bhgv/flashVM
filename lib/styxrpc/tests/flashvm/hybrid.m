# Copyright (c) 2026 bhgv
# EN: Compiled contract for native table graphs and simultaneous ordinary fields.
# RU: Компилированный контракт графов таблиц и одновременных изменений обычных полей.
include "bigtable.m";

Hybrid: module {
    Bag: adt {
        source: ref Bigtable->Handle; # Alias of the direct argument. / Алиас прямого аргумента.
        inner: ref Bigtable->Handle;  # Identity defined inside the ADT. / ID внутри ADT.
        stamp: int;                  # Changed with both tables. / Изменяется вместе с таблицами.
    };
    change: fn(tab: ref Bigtable->Handle, peers: array of ref Bigtable->Handle,
        bag: ref Bag): (ref Bigtable->Handle, ref Bigtable->Handle);
    reject: fn(tab: ref Bigtable->Handle, peers: array of ref Bigtable->Handle,
        bag: ref Bag): (ref Bigtable->Handle, ref Bigtable->Handle);
    empty: fn(): ref Bigtable->Handle;
};
