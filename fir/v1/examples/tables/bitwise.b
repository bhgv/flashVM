# Signed division/remainder and full-width bit operations in our Coco/R big profile.
implement Bitmath;


Bitmath: module
{
    value: fn(): big; # The standalone Dis harness captures the result without a Sys import.
};


# Keep the input in a table so optimized arithmetic still executes on runtime reads.
# Explicit string casts preserve the signed 64-bit key without passing through real.
value(): big
{
    values := table of big; # Opaque owner rooted in the entry frame.
    values.input = big -123456789012345; # Exact negative operand for signed division.
    input := values.input; # Capture the current value once as immutable big SSA data.
    quotient := input / 97; # Truncate toward zero, matching Dis divl.
    remainder := input % 97; # The remainder has the dividend's sign.
    restored := quotient * 97 + remainder; # Reconstruct the original input exactly.
    values[string restored] = big 42; # Store under its canonical decimal text key.
    toggled := restored ^ big -1; # Flip all 64 bits, without narrowing to int.
    back := ~toggled; # Complement again to recover the original signed bit pattern.
    masked := (back & big -1) | 0; # Full-width AND/OR preserve that bit pattern.
    return values[string masked]; # 42 in original/optimized reference and Dis execution.
}
