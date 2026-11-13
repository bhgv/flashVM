# Copyright (c) 2026 bhgv
# EN: Private native image tokens; finalizers enqueue bounded release metadata only.
# RU: Приватные native image tokens; finalizers ставят bounded release metadata.
Proxygc: module
{
    PATH: con "$Proxygc"; # Hosted native type identities, not wire or Dis64 kinds.


    # EN/RU: Opaque connection metadata; guest prefix carries no authority.
    # Непрозрачные connection metadata; guest prefix не несёт authority.
    Connection: adt
    {
        reserved: int; # Actual native payload is host-owned.
    };


    # EN/RU: One logical image owner; ordinary ref copies share this ownership.
    # Один logical image owner; обычные ref copies делят это ownership.
    Lease: adt
    {
        reserved: int; # Exact native type required, forged records refused.
    };


    # EN/RU: Create bounded metadata on this broker; nil on capacity failure.
    # Создать bounded metadata на этом broker; nil при capacity failure.
    open: fn(limit: int): ref Connection;


    # EN: Allocate a token before Retain; active=0 stages without remote ownership.
    # Return nil before remote effects on quota. Caller keeps connection alive.
    # RU: Создать token до Retain; active=0 означает staging без remote ownership.
    # При quota вернуть nil до remote effects. Caller удерживает connection.
    make: fn(connection: ref Connection, session: big, slot: int, generation: big, active: int): ref Lease;


    # EN/RU: Any-thread query of exact token; closed/released/nil returns zero.
    # Any-thread запрос точного token; closed/released/nil дают ноль.
    active: fn(lease: ref Lease): int;


    # EN/RU: Broker activates one staged lease after successful remote Retain; zero on refusal.
    # Broker активирует staged lease после remote Retain; ноль при отказе.
    activate: fn(lease: ref Lease): int;


    # EN/RU: Broker consumes one active lease before explicit Release; zero on refusal.
    # Broker потребляет active lease до explicit Release; ноль при отказе.
    consume: fn(lease: ref Lease): int;


    # EN/RU: Broker pops one finalized image handle; zero tuple when queue is empty.
    # Broker снимает handle finalized image; нулевой tuple при пустой очереди.
    next: fn(connection: ref Connection): (big, int, big);


    # EN/RU: Terminal broker close revokes every token and discards pending obligations.
    # Terminal broker close отзывает tokens и удаляет pending obligations.
    close: fn(connection: ref Connection): int;
};
