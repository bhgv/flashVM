# One geometry-only sender and native broker per stable pane identity.
Idebulk: module
{
    PATH: con "idebulk.dis"; # Portable sender implementation.
    Job: adt {
        ticket: int; # Exact scene generation, never a heap address.
        parts: array of string; # Shared immutable segments retained until all fragments are accepted.
        token: int; # Preview fence, zero when no native stroke has been adopted.
    };

    # Register one private byte-array output and spawn its sender. completed is
    # caller-rooted and receives slot after each complete transfer and shutdown.
    # Return nil on invalid dependencies/slot or native registration refusal.
    start: fn(codec: Uibridge, host: Idehost, slot: int,
              completed: chan of int): chan of ref Job;
};
