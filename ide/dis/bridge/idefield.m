# Single-owner, single-line Unicode field policy. Native widgets receive copied presentation values.
Idefield: module
{
    PATH: con "idefield.dis"; # Packaged reusable Limbo implementation.
    MaxCharacters: con 256; # Scalar budget; four-byte UTF-8 values still fit a common text envelope.
    MaxStamp: con 2147483647; # Generation never wraps onto an old clipboard field transaction.
    Ok: con 0; # Successful field operation, without any document revision/history change.
    RangeError: con 2; # Invalid state or selection endpoints.
    TooLarge: con 3; # Complete proposed value exceeds the scalar budget.
    Invalid: con 4; # Forbidden Unicode/control input or exhausted generation.


    # Exclusive mutable state; no reference may be shared with a worker or native view.
    State: adt
    {
        text: string; # [GC:] Owned immutable current field value.
        caret: int; # Unicode insertion endpoint, bounded by len text.
        anchor: int; # Other selection endpoint; reverse selections are allowed.
        stamp: int; # Local generation for field text/selection, separate from document revisions.
    };


    # Return an empty, exclusively owned field without loading native modules or performing I/O.
    create: fn(): ref State;


    # Replace the complete field and either select all or collapse at its end.
    # state is exclusive; text/all are values. Errors preserve text, endpoints and generation.
    set: fn(state: ref State, text: string, all: int): int;


    # Set an ordered or reversed scalar selection without editing the value; returns a status above.
    select: fn(state: ref State, anchor, caret: int): int;


    # Replace the current selection with a complete checked value; returns status, never a partial insertion.
    # Empty input removes a nonempty selection; clipboard empty-value no-op policy belongs to its caller.
    insert: fn(state: ref State, text: string): int;


    # Apply decoded Text or source-style keyboard navigation/deletion, using borrowed word policy.
    # control/shift are current Dis modifier values. Returns (status, handled); Enter/Escape/clipboard route above.
    # No document channel, Undo entry, native field or file is accessed.
    apply: fn(policy: Ideinput, state: ref State, event: ref Uibridge->Message, control, shift: int): (int, int);
};
