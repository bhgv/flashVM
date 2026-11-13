# Copyright (c) 2026 bhgv
# EN: Explicit network proxy images bound to one trusted connected FD incarnation.
# RU: Явные network proxy images, привязанные к incarnation доверенного connected FD.
Proxynet: module
{
    PATH: con "proxynet.dis"; # Client broker, one permanent Prog per connection.
    Disconnected: con -4; # Terminal byte/framing failure; sent effects may be unknown.
    Stale: con 5; # Image belongs to another connection incarnation.


    # EN/RU: An image fences original owner identity with externally unique connection epoch.
    # Image дополняет identity исходного owner уникальным извне connection epoch.
    Image: adt
    {
        high: big; # First 64 incarnation bits; not authentication.
        low: big; # Second 64 incarnation bits, never reused across reconnect/restart.
        owner: Proxyowner->Handle; # Original exported variable/function identity.
    };


    # EN/RU: Shared terminal flag; broker publishes before retiring its original thread.
    # Общий terminal flag; broker публикует до retirement исходного thread.
    State: adt
    {
        closed: int; # Zero while usable, one permanently after terminal outcome/idle lease.
    };


    # EN/RU: Local bounded mailbox and immutable connection incarnation.
    # Local bounded mailbox и immutable connection incarnation.
    Endpoint: adt
    {
        high: big; # Expected connection incarnation, first half.
        low: big; # Expected connection incarnation, second half.
        state: ref State; # Shared terminal publication, never reset on reconnect.
        mailbox: ref Proxyowner->Endpoint; # Shared only among clients inside this VM.
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
};
