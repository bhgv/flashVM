implement TextBits;

# Selecting a big result enables our Coco/R table/string/int extension profile.
# This example is compiled by the same C99 library services used by IDE clients.
TextBits: module
{
    value: fn(): big;
};


# Build an editor-style status label and verify aliases before returning 42.
# No arguments. The VM owns strings; frame root maps retain each live SSA value.
# All bit operations are signed32, even though the hosted Dis cells are 64-bit.
value(): big
{
    # Restrict the low byte to six bits, toggle a mask, then set bit one.
    # Numeric literals default to big here, so every int mask is explicit.
    flags := int 255;
    flags &= int 63;              # 63.
    flags ^= int 21;              # 42.
    flags |= int 2;               # Still 42.

    # Explicit string conversion prevents implicit numeric/text coercion.
    # Concatenation preserves left-to-right order and allocates a rooted result.
    label := "line " + string flags;
    original := label;            # Borrow the original immutable string value.
    label += " 😀";               # Rebind label; original still reads "line 42".

    # Equality compares contents, not storage addresses. Emoji count as one scalar.
    # The lazy conjunction skips later expressions if an earlier invariant fails.
    valid := (flags == int 42) && (original == "line 42") && (len label == int 9);
    if (valid) {
        # Slices use Unicode-scalar int bounds and retain their source ownership.
        digits := original[int 5:int 7];
        return big digits;        # Exact decimal conversion: 42.
    } else {
        return 0;                 # A failed invariant produces a distinct result.
    }
}
