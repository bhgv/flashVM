# Scalar capture publication prototype; no new Dis64 ABI or AS intrinsic yet.
Asincviews: module
{
    PATH: con "asincviews.dis"; # Ordinary isolated Limbo service.
    Tasks: con 8; # Stable task identities1..8; owner identity is0.
    Bindings: con 16; # Binding identities0..15, initially empty strings.
    Capacity: con 256; # Bounded global notification queue; refusal is atomic.
    Snapshot, Spawn, Write, BeforeFrame, AfterFrame, Subscribe,
        Drain, Discard, Retire, Shutdown: con iota; # Serialized owner commands.
    Ok, Invalid, Stale, Conflict, Full: con iota; # No partial changes on refusal.


    # A copied request. Values are immutable strings in this deliberately narrow prototype.
    Request: adt
    {
        id: int; # Nonnegative correlation identity.
        revision: int; # Exact service revision except Snapshot.
        operation: int; # One command above.
        task: int; # Target/subscriber0..Tasks; Spawn requires unused1..Tasks.
        binding: int; # Binding identity; ignored by phase/lifetime commands.
        value: string; # Write value, limited to4096 characters.
        reply: chan of Reply; # Required consumed acknowledgement.
    };


    # An immutable publication observation copied into the drain reply.
    Event: adt
    {
        subscriber: int; # View receiving the remote update.
        binding: int; # Stable captured binding identity.
        source: int; # Original writer task identity, owner0.
        version: int; # Writer version, not a new write caused by delivery.
        previous: string; # Previous published value at this subscriber.
        value: string; # New published value.
    };


    # Detached flat snapshots use task*Bindings+binding indexing; inactive slots are empty.
    Reply: adt
    {
        id: int; # Original correlation identity.
        revision: int; # Mutation revision for the next request.
        status: int; # Result above.
        phase: int; # Zero between frames, one during frame computation.
        active: array of int; # 1 live,0 never spawned,-1 retired; IDs never reused.
        values: array of string; # Copied scalar values, never mutable cell references.
        versions: array of int; # Local writer versions for inspection.
        sources: array of int; # Original writer identity by cell.
        dirty: array of int; # Own unpublished writes only; received versions are clean.
        events: array of Event; # Drain(target) only; other replies contain no events.
    };

    # Spawn a scheduled sole owner; caller receives every reply, including Shutdown.
    # No physical thread or frame hook is created; returned channel is caller-owned.
    start: fn(): chan of Request;
};
