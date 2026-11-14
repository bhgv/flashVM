# Copyright (c) 2026 bhgv
# EN: Explicit typed channel proxies to permanent native DisVM owners.
# RU: Явные typed channel proxies к постоянным native DisVM owners.
Proxyowner: module
{
    PATH: con "proxyowner.dis"; # Bytecode library built by the matching NC64 producer.
    Read, Write, Invoke, Drop, Stop, Retain, ReturnVariable, ReturnFunction, Add: con iota; # Local channel protocol, not network wire IDs.
    RoutedInvoke, ImportRoute, RoutedReturnVariable, Expose: con 9 + iota; # Routed/import wire operations; Expose remains owner-local.
    OK: con 0; # Completed effect, or successful explicit lifecycle action.
    OwnerError: con -1; # Callback raised; partial side effects remain possible.
    NotSent: con -2; # Timed out before this channel offer was delivered.
    Unknown: con -3; # Delivered; completion/outcome cannot be inferred after deadline.


    # EN/RU: Opaque object identity within one endpoint; no managed value pointer.
    # Непрозрачная object identity внутри endpoint, без managed value pointer.
    Handle: adt
    {
        session: big; # Fences foreign endpoints and reopened sessions.
        slot: int; # Bounded native object slot.
        generation: big; # Never-reused slot incarnation.
    };


    # EN: Managed callback result stays rooted through completion, including caller retirement.
    # RU: Managed callback result остаётся rooted до completion, включая retirement caller.
    RoutedResult: adt
    {
        source: ref Proxyref->Ref; # Exact live registered wrapper; never serialized directly.
    };


    # EN/RU: One immutable offer after send; caller owns the buffered reply channel.
    # Immutable offer после send; caller владеет buffered reply channel.
    Request: adt
    {
        operation: int; # Local owner action.
        target: Handle; # Original variable or captured function context.
        left: Handle; # First live ref argument when refs >= 1.
        right: Handle; # Second live ref argument when refs == 2.
        refs: int; # Number of same-owner ref arguments, zero to two.
        value: int; # Copied scalar input.
        routed: int; # Explicit proxy-ref callback, never raw foreign Cell arguments.
        first: ref Proxyref->Ref; # First routed variable owner, if used.
        second: ref Proxyref->Ref; # Second routed variable owner, if used.
        deadline: int; # Absolute caller budget for queue admission and routed accesses.
        publish: ref Proxyhost->Cell; # Owner-local cell to root for an opaque forwarded view.
        result: ref RoutedResult; # Optional callback output, owned through the request/reply wait.
        reply: chan of (int, int, Handle); # Buffered status/scalar/new export identity.
    };


    # EN/RU: Process-local typed mailbox; never serialize this channel as a capability.
    # Process-local typed mailbox; не сериализовать channel как capability.
    Endpoint: adt
    {
        requests: chan of ref Request; # Bounded owner mailbox, shared only in this VM.
    };


    # EN/RU: Initialize immutable builtin interfaces before spawning owners/clients.
    # Инициализировать immutable builtin interfaces до spawn owners/clients.
    init: fn();


    # EN: Serve original cell/capture with user handler, announcing endpoint and
    # handles on ready. This Prog remains their execution owner until Stop/retirement.
    # RU: Обслуживать исходные cell/capture через handler, передать endpoint/handles
    # в ready. Этот Prog остаётся execution owner до Stop/retirement.
    serve: fn(handler: Proxyhandler, cell, capture: ref Proxyhost->Cell,
              ready: chan of (ref Endpoint, Handle, Handle));


    # EN: Offer one call with 1..60000ms overall deadline; no replay. NotSent differs
    # from Unknown after delivery. Refs belong to this endpoint; return copied scalar.
    # RU: Предложить call с общим deadline 1..60000ms без replay. NotSent отличается
    # от Unknown после delivery. Refs принадлежат endpoint; result — scalar copy.
    # EN/RU: Direct nested proxy RPC is refused; routed variable access uses Proxyref.
    # Direct nested proxy RPC запрещён; routed variable access использует Proxyref.
    call: fn(endpoint: ref Endpoint, operation: int, target, left, right: Handle,
             refs, value, timeout: int): (int, int);


    # EN: Invoke original function, export its returned actual Cell with kind
    # Variable/Function and return status/handle. Unknown must never be replayed.
    # RU: Вызвать original function, export returned actual Cell с kind
    # Variable/Function; вернуть status/handle. Unknown не повторяется.
    fetch: fn(endpoint: ref Endpoint, target, left, right: Handle,
              refs, value, kind, timeout: int): (int, Handle);


    # EN: Invoke on original function owner using proxy refs to other owners.
    # Requests keep refs live; deadline is shared by nested variable-only accesses.
    # RU: Вызвать original function owner с proxy refs других owners.
    # Requests удерживают refs; nested variable-only accesses делят deadline.
    routed: fn(endpoint: ref Endpoint, target: Handle, left, right: ref Proxyref->Ref,
               refs, value, timeout: int): (int, int);


    # EN: Authorize function and root an owner-local Cell without invoking user code.
    # Return a new variable handle; only trusted local coordinators supply the Cell.
    # RU: Авторизовать function и укоренить owner-local Cell без user callback.
    # Вернуть new variable handle; Cell передают лишь trusted local coordinators.
    expose: fn(endpoint: ref Endpoint, function: Handle, cell: ref Proxyhost->Cell,
               timeout: int): (int, Handle);


    # EN/RU: Registry variants preserve an already established absolute local deadline.
    # Registry variants сохраняют уже установленный absolute local deadline.
    routeduntil: fn(endpoint: ref Endpoint, target: Handle, left, right: ref Proxyref->Ref,
                    refs, value, deadline: int): (int, int);


    # EN/RU: Publish a stub under the original absolute local deadline.
    # Публиковать stub под original absolute local deadline.
    exposeuntil: fn(endpoint: ref Endpoint, function: Handle, cell: ref Proxyhost->Cell,
                    deadline: int): (int, Handle);


    # EN: Run original callback with live refs and return status/Ref under existing deadline.
    # The registry must validate the returned wrapper and acquire peer ownership separately.
    # RU: Исполнить original callback с live refs; вернуть status/Ref до existing deadline.
    # Registry отдельно проверяет returned wrapper и приобретает peer ownership.
    routedfetchuntil: fn(endpoint: ref Endpoint, target: Handle, left, right: ref Proxyref->Ref,
                         refs, value, deadline: int): (int, ref Proxyref->Ref);


    # EN/RU: Retain a result capability with no renewal of the original callback deadline.
    # Retain result capability без обновления original callback deadline.
    retainuntil: fn(endpoint: ref Endpoint, target: Handle, deadline: int): int;

    # EN/RU: Scalar/lifecycle admission preserves an existing absolute local deadline.
    # Scalar/lifecycle admission сохраняет existing absolute local deadline.
    calluntil: fn(endpoint: ref Endpoint, operation: int, target, left, right: Handle,
                  refs, value, deadline: int): (int, int);


    # EN/RU: Typed result admission preserves an existing absolute local deadline.
    # Typed result admission сохраняет existing absolute local deadline.
    fetchuntil: fn(endpoint: ref Endpoint, target, left, right: Handle,
                   refs, value, kind, deadline: int): (int, Handle);

};
