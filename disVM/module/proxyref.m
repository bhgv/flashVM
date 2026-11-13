# Copyright (c) 2026 bhgv
# EN: Typed bounded variable access; no foreign Cell pointers or function invocation.
# RU: Typed bounded variable access; без foreign Cell pointers и function invocation.
include "proxygc.m";
Proxyref: module
{
    PATH: con "proxyref.dis"; # Variable-only access library, usable inside routed callbacks.
    Read, Write, Add, Close: con iota; # Private access mailbox operations, not wire opcodes.
    NotSent: con -2; # Access not offered before its absolute deadline.
    Unknown: con -3; # Offered access may have completed an effect.
    Disconnected: con -4; # Actor retired or lease revoked.
    Stale: con 5; # Token belongs to another actor or is no longer active.


    # EN/RU: Shared terminal state, published by this ref's original access actor.
    # Общий terminal state, публикуемый original access actor этого ref.
    State: adt
    {
        closed: int; # Terminal once set; never reset for replacement refs.
    };


    # EN/RU: Local typed capability; ordinary aliases share exactly one GC owner.
    # Local typed capability; ordinary aliases разделяют один GC owner.
    Ref: adt
    {
        requests: chan of ref Access; # Bounded private variable-only mailbox.
        state: ref State; # Terminal publication owned by the actor.
        lease: ref Proxygc->Lease; # Exact native metadata owner, never sent over wire.
    };


    # EN/RU: Immutable access offer keeps the ref live until buffered completion.
    # Immutable access offer удерживает ref до buffered completion.
    Access: adt
    {
        source: ref Ref; # Mapped logical owner through access completion.
        operation: int; # Read/Write/Add/Close only; no callback dispatch.
        value: int; # Scalar input for Write/Add.
        deadline: int; # Absolute local monotonic deadline; never renewed by queueing.
        reply: chan of (int, int); # One-slot completion survives caller retirement.
    };


    # EN/RU: Load immutable clock/lifecycle interfaces before sharing with callbacks.
    # Загрузить immutable clock/lifecycle interfaces до sharing с callbacks.
    init: fn();


    # EN: Offer one variable access by absolute deadline (at most 60000ms away).
    # Routed callbacks may wait here; arbitrary nested function RPC remains separate.
    # No effect retry; Unknown does not prove cancellation. Alias Close revokes all.
    # RU: Предложить variable access до absolute deadline (не далее 60000ms).
    # Routed callback может ждать здесь; arbitrary nested function RPC — отдельно.
    # Нет effect retry; Unknown не доказывает отмену. Close отзывает все aliases.
    call: fn(source: ref Ref, operation, value, deadline: int): (int, int);
};
