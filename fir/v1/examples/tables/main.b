# Our Coco/R Limbo table subset, lowered to FIR without the legacy compiler.
implement Tables;


Tables: module
{
    value: fn(): big; # Restricted tutorial entry; the hosted FIR runner observes this result.
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
    alias.total = big 42; # Literal string key: equivalent to alias["total"].
    return t.total; # Observe the aliased write through the original owner.
}
