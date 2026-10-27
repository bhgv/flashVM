implement ValueIteration;

# Choose the Coco/R Limbo big/table profile shared by CLI and C99 IDE clients.
ValueIteration: module
{
    value: fn(): big; # Both VM modes and reference execution return 42.
};


# One binding receives the value, not the key. The omitted key has no synthetic
# source name or debug declaration. Use the existing two-name form when needed.
value(): big
{
    entries := table of big {first=7, second=7};
    total := big 0;
    for (amount in entries) {
        total += amount;
    }

    # A string yields Unicode rune ints, not UTF-8 bytes. The captured immutable
    # string is still traversed even if another local is rebound in the body.
    text := "A😀";
    for (rune in text) {
        total += 7;
        text = "";
    }

    # keysof yields a snapshot of keys. Here the values of that snapshot are key
    # capabilities; they can address the original table without conversion or
    # exporting a native heap pointer. The table payload read remains live.
    for (key in keysof entries) {
        total += entries[key];
    }

    # Each of the three traversals contributes 14. Pair iteration is unchanged:
    # for (key, amount in entries) { ... } binds both names in the same scope.
    return total;
}
