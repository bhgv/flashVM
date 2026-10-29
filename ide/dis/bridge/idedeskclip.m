# Session clipboard service: copied immutable text, no native/OS clipboard handle.
Idedeskclip: module
{
    PATH: con "idedeskclip.dis"; # Matching independently scheduled implementation.
    MaxBytes: con 65536; # Bound UTF-8 payloads; never a bound on document capacity.
    Get, Put, Clear, Shutdown: con iota; # Ordered channel operations.
    Ok, Conflict, TooLarge, Invalid, Limit: con iota; # Explicit value admission statuses.

    Request: adt
    {
        id: int; # Nonnegative correlation, one reply for each admitted endpoint.
        operation: int; # Get/Put/Clear/Shutdown.
        expected: int; # Exact current generation for Put/Clear, ignored by Get/Shutdown.
        text: string; # Copied Put value; no pointer or source authority.
        reply: chan of Reply; # Required unique acknowledgement channel.
    };

    Reply: adt
    {
        id: int; # Original request identity.
        status: int; # Admission result above; errors preserve the current value.
        generation: int; # Monotonic value identity, never wrapped.
        text: string; # Immutable copied session text, independently usable after replacement.
        bytes: int; # Complete UTF-8 extent, at most MaxBytes.
    };

    # Spawn one private session owner. No parameters; return its ordinary channel.
    # Put/Clear require a fresh Get generation. Shutdown acknowledgement is terminal.
    # No filesystem, GUI, platform clipboard or external compiler is accessed.
    start: fn(): chan of Request;
};
