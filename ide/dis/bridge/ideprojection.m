# Explicit inspector capabilities; targets stay in their owning Dis process.
Ideprojection: module
{
    PATH: con "ideprojection.dis"; # Portable owner service, with no native heap pointers.
    Describe, Read, Write, Invoke, Pause, Resume, Release: con iota; # Ordered owner commands.
    Ok, Invalid, Stale, Paused, Denied: con iota; # Single terminal reply status.
    MaxText: con 4096; # Bounded Unicode characters in a write request.


    # Inspector request. Retain and consume reply; do not mutate the record while pending.
    Request: adt
    {
        id: int; # Nonnegative caller correlation, not a global RPC identifier.
        epoch: int; # Exact positive owner incarnation; rejects a replaced target.
        operation: int; # One command above; only Invoke executes the target method.
        member: string; # Explicit field name or method; never evaluate source expressions.
        number: int; # Typed scalar write or invocation argument.
        text: string; # Typed text write, bounded before mutation.
        permit: int; # Invoke requires explicit caller consent, exactly 1.
        site: int; # Caller source occurrence, echoed independently of request ID.
        reply: chan of Reply; # Dedicated rendezvous; caller must receive one reply.
    };


    # Copied inspector result; no target reference or mutable field array escapes.
    Reply: adt
    {
        id: int; # Original correlation ID.
        site: int; # Original exact source occurrence ID.
        status: int; # Ok or explicit rejection; rejected writes have no effect.
        epoch: int; # Current owner incarnation.
        revision: int; # Monotonic admitted mutation count.
        number: int; # Explicit integer projection or method result.
        text: string; # Explicit string projection or declaration list.
    };


    # Create a sample owner with scalar fields and an add method; epoch must be positive.
    # Returns nil on invalid epoch; otherwise spawns a process and returns its request channel.
    # [GC:] Target state lives only in that process. Release acknowledges then exits it.
    start: fn(epoch: int): chan of ref Request;
};
