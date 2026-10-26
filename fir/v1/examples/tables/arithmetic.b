# Our Coco/R big profile: exact arithmetic emitted as existing 64-bit Dis ALU instructions.
implement Bigmath;


Bigmath: module
{
    value: fn(): big; # The restricted harness captures one big result, without a Sys dependency.
};


# Demonstrate wrap, negation and arithmetic on values read from a mutable table.
# Every value stays signed 64-bit; no intermediate is an AS Number or Limbo real.
value(): big
{
    maximum := big 9223372036854775807; # Largest signed big literal.
    wrapped := maximum + 1; # Modulo 2^64 gives -9223372036854775808.
    flipped := -wrapped; # Negating that minimum wraps to the same bit pattern.
    key := string flipped; # Explicit decimal copy keeps all 64 bits in this text key.
    values := table of big; # Fresh mutable owner, held in the Dis frame GC map.
    values[key] = big 7; # Store the first value under the exact decimal string.
    values[key] = values[key] + 5 * values[key]; # Capture reads in order; multiplication binds first.
    return values[key]; # The result is exactly 42 in reference, interpreter and JIT modes.
}
