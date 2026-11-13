# Copyright (c) 2026 bhgv
# EN: Explicit network proxy images bound to one trusted connected FD incarnation.
# RU: Явные network proxy images, привязанные к incarnation доверенного connected FD.
include "proxygc.m";
Proxynet: module
{
    PATH: con "proxynet.dis"; # Client broker, one permanent Prog per connection.
    Disconnected: con -4; # Terminal byte/framing failure; sent effects may be unknown.
    GCProgress: con 6; # Local broker maintenance, never a wire operation.
    Stale: con 5; # Image belongs to another connection incarnation.


    # EN/RU: An image fences original owner identity with externally unique connection epoch.
    # Image дополняет identity исходного owner уникальным извне connection epoch.
    Image: adt
    {
        high: big; # First 64 incarnation bits; not authentication.
        low: big; # Second 64 incarnation bits, never reused across reconnect/restart.
        owner: Proxyowner->Handle; # Original exported variable/function identity.
        lease: ref Proxygc->Lease; # One GC-finalized remote owner; aliases share this token.
    };


    # EN/RU: Shared terminal flag; broker publishes before retiring its original thread.
    # Общий terminal flag; broker публикует до retirement исходного thread.
    State: adt
    {
        closed: int; # Zero while usable, one permanently after terminal outcome/idle lease.
    };


    # EN/RU: Local completed outcome; image is nonempty only for an explicit clone.
    # Local completed outcome; image непуст лишь для explicit clone.
    Reply: adt
    {
        status: int; # Scalar operation outcome.
        value: int; # Copied scalar result.
        image: Image; # Newly retained clone, or empty image.
    };


    # EN/RU: Immutable local offer keeps target/ref leases alive until broker completion.
    # Immutable local offer удерживает target/ref leases до broker completion.
    Request: adt
    {
        operation: int; # Owner operation, including private clone/Retain.
        target: Image; # Target logical owner, including GC token.
        left: Image; # First ref owner if used.
        right: Image; # Second ref owner if used.
        refs: int; # Zero to two used refs.
        value: int; # Copied scalar input.
        reply: chan of Reply; # One-slot completion, independent of caller survival.
    };


    # EN/RU: Local bounded mailbox and immutable connection incarnation.
    # Local bounded mailbox и immutable connection incarnation.
    Endpoint: adt
    {
        high: big; # Expected connection incarnation, first half.
        low: big; # Expected connection incarnation, second half.
        state: ref State; # Shared terminal publication, never reset on reconnect.
        requests: chan of ref Request; # Shared only among clients inside this VM.
    };


    # EN/RU: Load immutable codec/IO/owner interfaces before sharing with callers.
    # Загрузить immutable codec/IO/owner interfaces до sharing с callers.
    init: fn();


    # EN: Run a client broker on this permanent Prog, announcing endpoint/images
    # after checked Hello. ready status is nonzero with nil endpoint on refusal.
    # Borrow exclusive-direction FDs until Stop/retirement.
    # high/low are a nonzero externally provisioned, never-reused epoch; timeout
    # bounds wire exchanges and idle lifetime. Closed broker retires and rejects new calls.
    # RU: Запустить client broker на этом Prog; после Hello передать endpoint/images.
    # FD заимствованы до Stop/retirement; направления принадлежат только broker.
    # high/low — ненулевой уникальный epoch извне; timeout ограничивает wire exchange.
    # После разрыва/idle lease broker завершается и отказывает новым calls.
    start: fn(input, output: ref Sys->FD, high, low: big, timeout: int,
              ready: chan of (int, ref Endpoint, Image, Image));


    # EN: Validate image incarnation before any offer, then use local owner call
    # deadlines. Unknown never authorizes replay; explicit Stop joins broker later.
    # RU: Проверить incarnation image до offer, затем применить local call deadline.
    # Unknown не разрешает replay; explicit Stop предшествует join broker.
    call: fn(endpoint: ref Endpoint, operation: int, target, left, right: Image,
             refs, value, timeout: int): (int, int);


    # EN: Retain a distinct remote owner; ordinary Image/ref copies share one lease.
    # Allocation/quota refusal occurs before Retain. Unknown is never replayed.
    # RU: Удержать отдельный remote owner; обычные Image/ref copies делят lease.
    # Allocation/quota refusal предшествует Retain. Unknown не повторяется.
    clone: fn(endpoint: ref Endpoint, source: Image, timeout: int): (int, Image);


    # EN/RU: Observe acknowledged automatic GC releases without forcing/stopping the heap.
    # Наблюдать acknowledged automatic GC releases без принудительной остановки heap.
    collected: fn(endpoint: ref Endpoint, timeout: int): (int, int);
};
