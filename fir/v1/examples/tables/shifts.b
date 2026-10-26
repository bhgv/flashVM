# Exact signed long shifts in our bounded Coco/R big-result profile.
# Count values use the existing fixed-width i32 int semantics even on an NC64 host.
implement ShiftExample;

ShiftExample: module {
    value: fn(): big; # The usual capture entry returns one selected big result.
};

# Recover MIN from its sign, then return 42 using typed casts and hybrid string keys.
# Every table/string/int value remains local to its VM; no implicit owner RPC occurs.
value(): big
{
    # Reading from the table keeps the long operand dynamic for the native JIT.
    t := table of big;
    t.input = big -9223372036854775808;

    # The shift count must be explicitly int; narrowing keeps the signed low 32 bits.
    count: int = int 63;
    sign := t.input >> count; # Arithmetic right shift yields -1, not logical +1.
    restored := sign << count; # Unsigned-bit left shift recovers the exact MIN bits.

    # Reflection distinguishes int from big. String conversion uses its signed value.
    t["int"] = big 42;
    t[string count] = t[typeof count];

    # This comparison stays dynamic because the table value is read at execution time.
    if (restored == t.input) {
        # Explicit int narrowing wraps 2^32 to zero; explicit big promotes it back.
        return t["63"] | big (int 4294967296);
    } else {
        # Counts outside 0..63 raise invalid argument; this arm must not execute here.
        return 1 << int 64;
    }
}
