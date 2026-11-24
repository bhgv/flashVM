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
    start: fn(codec: Uibridge, host: Unibus, slot: int,
              completed: chan of int): chan of ref Job;

    # EN: Send declared geometry and bounded fragments in scene order on output.
    # RU: Передать объявление и ограниченные фрагменты геометрии по output в порядке сцены.
    publish: fn(codec: Uibridge, output: chan of array of byte, ticket, slot, revision: int,
                parts: array of string);
};
