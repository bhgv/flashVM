# Channel-only inspector view model. Rendering consumes copied rows, never target objects.
Ideinspect: module
{
    PATH: con "ideinspect.dis"; # Pure Dis view model; transport workers remain external.
    Select, Begin, Apply, Snapshot, Shutdown: con iota; # View-model operations.
    Ok, Invalid, Stale, Limit: con iota; # Explicit admission results.
    MaxRows: con 16; # Bounded simultaneous watch rows.


    # Independent row snapshot. A zero ticket means no operation has been started.
    Row: adt
    {
        epoch: int; # Selected target incarnation.
        member: string; # Explicit selected field/method, never evaluated as source.
        ticket: int; # Unique service-lifetime request ticket; never recycled.
        pending: int; # One while awaiting an admitted result.
        status: int; # Owner status, meaningful after a successful Apply.
        revision: int; # Last accepted owner revision.
        number: int; # Last accepted typed integer projection.
        text: string; # Last accepted string projection.
        value: ref Idevalue->Value; # Independent explicit value/reference for the current row.
    };


    # All clients must consume one reply. Apply borrows result until acknowledgement.
    Request: adt
    {
        operation: int; # One view-model operation above.
        row: int; # Zero-based row slot, bounded by MaxRows.
        epoch: int; # Select: positive new target incarnation.
        member: string; # Select: explicit member name, bounded to 256 characters.
        ticket: int; # Apply: exact Begin ticket captured by its transport worker.
        result: ref Ideprojection->Reply; # Apply: copied owner reply; no target pointer.
        reply: chan of Reply; # Required single result rendezvous.
    };


    # Owned copy for rendering, or nil for a rejected request.
    Reply: adt
    {
        status: int; # View-model admission result, distinct from owner status.
        value: ref Row; # Independent row record; caller may retain without aliasing model state.
    };


    # Spawn a row owner and return its channel. No native UI or target operation runs here.
    # Callers must route Select/Begin/Apply/Snapshot through this endpoint and drain replies.
    start: fn(): chan of ref Request;
};
