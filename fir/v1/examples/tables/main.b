# Our Coco/R Limbo table subset, lowered to FIR without the legacy compiler.
implement Tables;


Tables: module
{
    value: fn(): big; # Restricted entry: native Dis captures the result; the C reference runner prints it.
};


# Demonstrate dense/sparse keys, string sugar, shared table aliases and copied slices.
# All stored values are big; keys are numeric or string. The returned value is exactly 42.
value(): big
{
    t: table of big; # Typed declaration; reading before initialization is rejected.
    t = table of big; # Create a fresh mutable owner.
    t[0] = big 12; # The numeric sequence begins at zero.
    t[1] = big 24; # Its contiguous length is now two.
    t["total"] = big 36; # Dictionary entries do not extend that numeric sequence.
    keys := keysof t; # Independent snapshot containing the three existing keys.
    t[7] = big 70; # Sparse numeric key: does not fill the gap at index two.
    prefix := len t; # Still two; internal dense/hash placement is unobservable.
    count := len keys; # Still three: later writes do not mutate this snapshot.
    part := t[0:2]; # New zero-based table containing only the selected numeric entries.
    part[0] = big 99; # Does not modify t[0], which remains twelve.
    alias := t; # A table assignment retains the same mutable owner.
    key := keys[2]; # Checked snapshot key: the two numeric keys precede literal "total".
    keys = keysof part; # Replacing this local does not invalidate key's retained snapshot.
    alias[key] = big 42; # Write through the original string key, without converting its namespace.
    return t[key]; # Observe the aliased write; key retains its immutable snapshot until retirement.
}
