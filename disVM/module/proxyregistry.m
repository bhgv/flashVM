# Copyright (c) 2026 bhgv
# EN: Connection-scoped forwarding views for explicitly registered source images.
# RU: Connection-scoped forwarding views для явно зарегистрированных source images.
Proxyregistry: module
{
    PATH: con "proxyregistry.dis"; # Private managed registry; never serializes its channels.


    # EN/RU: Server-authorized source template, copied before the connection starts.
    # Server-authorized source template, копируется до начала connection.
    Source: adt
    {
        endpoint: ref Proxynet->Endpoint; # Borrowed live source connection.
        image: Proxynet->Image; # Independently owned registration image.
    };


    # EN/RU: One forwarded variable capability in this function owner's namespace.
    # Одна forwarded variable capability в namespace этого function owner.
    View: adt
    {
        owner: Proxyowner->Handle; # Native rooted stub; never exposed as a local Cell.
        source: ref Proxyref->Ref; # Owns its independent source clone through actor GC.
        owners: int; # Counted client images; native Retain/Drop mirror this count.
    };


    # EN/RU: One connection owns registry mutation and all actor completion channels.
    # Одна connection владеет registry mutation и всеми actor completion channels.
    Context: adt
    {
        sources: array of ref Source; # Snapshot of at most sixteen configured sources.
        views: array of ref View; # At most sixteen simultaneously exported native stubs.
        actors: array of chan of int; # Bounded issued actors, including retired views.
        issued: int; # Monotonic count, never reused to hide unjoined actors.
        endpoint: ref Proxyowner->Endpoint; # Original function execution owner.
        timeout: int; # Finite source exchange/idle/cleanup budget.
        closed: int; # Terminal after cleanup begins.
    };


    # EN/RU: Initialize immutable dependencies before connection sessions.
    # Инициализировать immutable dependencies до connection sessions.
    init: fn();


    # EN/RU: Snapshot configured sources; nil on absent/invalid configuration.
    # Скопировать configured sources; nil при absent/invalid configuration.
    create: fn(sources: array of ref Source, endpoint: ref Proxyowner->Endpoint, timeout: int): ref Context;


    # EN: Interpret forwarding operations before ordinary native owner dispatch.
    # Return handled/status/value/returned handle/result kind. No effect replay.
    # RU: Обработать forwarding operations до ordinary native owner dispatch.
    # Вернуть handled/status/value/returned handle/result kind. Без effect replay.
    dispatch: fn(context: ref Context, operation: int, target, left, right: Proxyowner->Handle,
                 refs, value, deadline: int): (int, int, int, Proxyowner->Handle, int);


    # EN/RU: Revoke views after original callback owner joins, then join every source actor.
    # Отозвать views после join original callback owner, затем join всех source actors.
    close: fn(context: ref Context);
};
