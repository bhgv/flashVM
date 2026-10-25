# Shared interface for a Dis implementation or an external AS/C implementation.
# The host must resolve the full descriptor before binding either engine.


# Arithmetic exposes values; no native frame or heap pointer crosses this interface.
Arithmetic: module                         # Module identity used by both generated adapter directions.
{                                          # Begin the public module declarations.


    # Add two signed 32-bit values using Limbo int wraparound.
    # Parameters: a and b are copied input values. Returns their signed i32 sum.
    # An AS implementation must wrap explicitly instead of returning an unbounded Number.
    add: fn(a, b: int): int;                # Scalar result; no object ownership transfers.


    # Construct a greeting from a string value.
    # Parameters: name is a Limbo string value. Returns a newly constructed string.
    # The checked bridge copies/serializes strings rather than sharing heap addresses.
    greet: fn(name: string): string;        # The returned string belongs to the receiving VM's heap.
};                                         # Finish the interface; this is a header, not an implementation.
