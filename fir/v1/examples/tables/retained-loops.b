implement RetainedLoops;

# A big result selects the typed hybrid-table source profile.
RetainedLoops: module
{
    value: fn(): big; # The reference runner and both DisVM modes return 42.
};


# Rebind retained values without changing existing aliases or captured receivers.
# Private frame cells belong to this function; no native pointer or cell handle
# is exported through a channel. Each payload still has its ordinary source type.
value(): big
{
    current := table of big {answer=40};
    original := current; # An alias to the first table, not to current's cell.
    text := "A";
    keys := keysof current;
    key := keys[0]; # A snapshot capability retains its key owner and position.

    for (index := int 0; index < int 2; index++) {
        # The RHS reads the old table before replacing current's retained owner.
        # original continues to refer to the first table, whose answer is 40.
        current = table of big {answer=current.answer+1};
        text = text + "😀";
        keys = keysof current;
        key = keys[0];
    }

    # The receiver is captured once. Replacing text below cannot change the
    # string being traversed: it is still A followed by two supplementary runes.
    visited := big 0;
    for (index, rune in text) {
        text = "changed";
        visited++;
    }

    # Ordinary reads recover typed payloads from the cells. Key access retains
    # the snapshot identity; current[key] resolves the string key "answer".
    if (original.answer == 40 && current[key] == 42 && visited == 3 &&
        text == "changed" && typeof current == "table of big") {
        return current[key];
    } else {
        return 0;
    }
}
