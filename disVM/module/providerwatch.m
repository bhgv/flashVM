# Copyright (c) 2026 bhgv
# EN: Ordinary-FD provider event reading, explicit ack and one-record reads.
# RU: Provider events через ordinary FD, explicit ack и one-record reads.
Providerwatch: module
{
    PATH: con "providerwatch.dis"; # Pure Limbo library, not a native builtin.
    EventReady: con 1; # A captured event needs caller state refresh and explicit ack.
    Idle: con 0; # No pending event, never file EOF.
    Error: con -1; # File/protocol failure; event is not acknowledged.
    Rights, Namespace, Content, Connection, Load, Network: con 1 << iota; # Native journal class bits.
    All: con 63; # All supported classes.
    Resource: con 1; # Opaque resource token is meaningful, including all-zero token.


    # EN/RU: Copied event fields, unsigned IDs remain opaque16 hex strings.
    # Копии event fields; unsigned IDs остаются opaque16 hex strings.
    Event: adt
    {
        ticket: string; # FD-local acknowledgement token, not a provider address.
        session: string; # Connection epoch of the captured ticket.
        generation: string; # Captured source report watermark.
        kinds: int; # Changed class mask.
        flags: int; # Optional resource scope.
        resource: string; # Opaque provider-local ID, not a path/pointer.
    };


    # EN/RU: One observer owns an ordinary ORDWR descriptor; do not share calls
    # concurrently without your own channel/lock. FD lifetime is managed normally.
    # Observer владеет ORDWR FD; concurrent calls требуют своей сериализации.
    Watch: adt
    {
        fd: ref Sys->FD; # Typed descriptor capability, retained until Watch retirement.
        pending: string; # Most recently read ticket token; nil after ack/filter change.
    };


    # EN/RU: Initialize Sys before sharing this module; raise if unavailable.
    # Загрузить Sys до передачи модуля другим guests; raise при отказе.
    init: fn();


    # EN/RU: Open an explicitly mounted event file, set classes1..63; return status/owner.
    # Открыть явно mounted event file, задать classes1..63; вернуть status/owner.
    open: fn(path: string, kinds: int): (int, ref Watch);


    # EN/RU: Read one complete frame; return1/event,0/idle,-1/error. No acknowledgement.
    # Прочитать frame; 1/event,0/idle,-1/error. Без acknowledgement.
    next: fn(watch: ref Watch): (int, ref Event);


    # EN/RU: Ack this watch's delivered ticket only after successful state reread.
    # Подтвердить ticket этого watch после successful state reread;0 success,-1 error.
    ack: fn(watch: ref Watch, event: ref Event): int;


    # EN/RU: Change selected classes, discarding local ticket without acknowledging.
    # Изменить classes; сбросить local ticket без acknowledgement.0 success,-1 error.
    mask: fn(watch: ref Watch, kinds: int): int;

};
