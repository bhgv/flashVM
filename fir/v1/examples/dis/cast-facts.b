# Our Coco/R Limbo frontend compiles this typed-table example to FIR and Dis.
implement CastFacts; # Match the implementation to the exported interface.
CastFacts: module { # Keep every table and scalar owned by one invocation.
    value: fn(): big; # Export a full-width payload for the Dis capture wrapper.
}; # End the interface before the function implementation.
value(): big { # Both raw and optimized execution return the same big forty-two.
    Number: type int; # Use an exact signed32 alias within the big-capable profile.
    samples := table of big {input = big -43}; # Checked table reads remain observable.
    narrowed: Number = Number samples.input; # Retain only the low32 payload bits.
    nonnegative := narrowed & Number 2147483647; # Prove the signed32 sign bit is zero.
    widened := big nonnegative; # Sign extension now proves all upper32 bits zero.
    high := widened >> Number 63; # Arithmetic shift therefore has the exact result zero.
    return high + big 42; # Folding this result must not remove a table-read failure.
} # The local table is released by the existing runtime ownership rules.
