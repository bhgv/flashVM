# Bounded clipboard transfer policy, owned by one Dis controller process.
Ideclipboard: module
{
    PATH: con "ideclipboard.dis"; # Packaged beside the matching editor controller.
    MaxBytes: con 65536; # Maximum copied clipboard UTF-8 bytes, independent of document capacity.
    PayloadBytes: con 1016; # Envelope text budget after its eight-byte transfer identifier.
    None: con 0; # Idle state; no native result may authorize an edit.
    Copy: con 1; # Read-only source selection copy, completed by a native acknowledgement.
    Cut: con 2; # Delete the captured range only after a successful native copy acknowledgement.
    Paste: con 3; # Stage all UTF-8 bytes before one document Replace.
    Ok, Conflict, RangeError, TooLarge, Invalid: con iota; # Copied Idecore status subset for this value boundary.


    # One exclusive controller owns the pending transaction and its private staging buffer.
    State: adt
    {
        action: int; # None/Copy/Cut/Paste, reset on completion or unrelated input.
        ticket: int; # Original completed-view ticket, stable across fragment acknowledgements.
        revision: int; # Source revision captured before native clipboard access.
        first: int; # Ordered Unicode source selection start captured for this transaction.
        last: int; # Ordered Unicode source selection end, excluded.
        total: int; # Expected byte extent; -1 before the first fragment.
        used: int; # Next required byte offset, never a Unicode position.
        bytes: array of byte; # [GC:] Private staging root, released on reset; never crosses heaps.
    };


    # Return a fresh exclusively owned idle transfer; no channel, native access or I/O occurs.
    create: fn(): ref State;


    # Retire pending state and staging storage; state is exclusive and may be nil.
    # Returns nothing; already copied immutable result strings remain independently owned.
    reset: fn(state: ref State);


    # Begin a checked action against a completed source view, replacing any pending transfer.
    # Values are copied; returns one for valid action/identity/range, zero with idle state otherwise.
    begin: fn(state: ref State, action, ticket, revision, first, last: int): int;


    # Admit one decoded clipboard fragment: flags=revision, first=byte offset, second=total,
    # text=eight ASCII ticket digits plus Unicode body. Caller supplies the common validated codec.
    # Returns (status, ready, text): status 0/1/2/3/4 matches Ok/Conflict/Range/TooLarge/Invalid.
    # Only ready=1 returns the complete immutable paste value; errors reset private staging.
    chunk: fn(codec: Uibridge, state: ref State, event: ref Uibridge->Message): (int, int, string);
};
