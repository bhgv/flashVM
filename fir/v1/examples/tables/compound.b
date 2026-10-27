implement CompoundTables;

# Our Coco/R frontend selects the exact big/table profile from this interface.
# CLI and C99 library callers use the same grammar and semantic actions.
CompoundTables: module
{
    value: fn(): big;
};


# Return 42 after typed compound updates through a table alias and a key snapshot.
# No arguments. The table/key owners remain local to this VM and retire through GC.
# Updates capture the location and old value before the RHS, then commit once.
# They are not transactions or atomic operations on remote projections.
value(): big
{
    document := table of big;
    document.score = big 10;

    # An alias retains the same mutable owner, rather than copying its contents.
    # A key snapshot preserves numeric/text key identity independently of mutation.
    alias := document;
    keys := keysof document;

    # Each receiver and key is evaluated once. This selects the original score key.
    alias[keys[0]] += 2;  # 10 + 2 = 12.
    alias.score *= 4;     # 12 * 4 = 48.
    alias.score -= 6;     # 48 - 6 = 42.

    # Signed big division truncates toward zero; remainder retains dividend sign.
    # These identities keep this tutorial's result stable while showing the syntax.
    alias.score /= 1;
    alias.score %= 100;

    # Full-width bitwise updates retain exact modulo-2^64 big semantics.
    alias.score &= 63;    # Keep the low six bits; 42 fits completely.
    alias.score |= 0;     # No additional bits are set.
    alias.score ^= 0;     # No bits are toggled.

    # Shift counts have type int, unlike the big value stored in the table.
    # Counts outside 0..63 raise before any destination store.
    alias.score <<= int 1; # 42 becomes 84.
    alias.score >>= int 1; # Arithmetic right shift returns 42.

    if (document.score == 42) {
        # Inferred scalar locals also support these exact big updates.
        answer := document.score;
        answer += 0;
        return answer;
    } else {
        # A wrong update must not silently produce a successful tutorial result.
        return 1 / 0;
    }
}
