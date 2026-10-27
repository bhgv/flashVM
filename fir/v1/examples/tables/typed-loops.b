implement TypedLoops;

# The big result selects the typed hybrid-table compiler profile.
TypedLoops: module
{
    value: fn(): big; # The reference runner and both DisVM modes return 42.
};


# Mix typed counters, a boolean stop flag and two kinds of immutable traversal.
# No VM object pointer crosses a compiler/VM boundary. Strings count Unicode
# scalars: the supplementary-plane character below is one rune, not four bytes.
value(): big
{
    total := big 0;
    running := 1 == 1; # Bool is inferred from a comparison, without truthiness.
    counter := int 0; # This remains int even when backed by a private frame cell.

    while (running) {
        counter++;
        total += big counter; # Widen explicitly: big and int do not mix silently.
        if (counter == int 3) {
            running = 1 == 0; # This assignment changes the next loop test.
        }
    }

    # String traversal returns an int rune index and an int Unicode scalar.
    # The three indexes are 0, 1, 2 regardless of their UTF-8 byte lengths.
    for (index, rune in "A😀Я") {
        total += big index;
    }

    values := table of big {left=10, right=23};
    snapshot := keysof values; # Owns the key sequence independently of values.
    for (index, key in snapshot) {
        # Snapshot indexes are big. Each key retains its string/numeric identity.
        # Mutating values does not invalidate the independent key sequence.
        total += values[key];
        delete values[key];
    }

    # 1+2+3 + 0+1+2 + 10+23 = 42. Every table entry has been removed.
    if (total == 42 && countof values == 0 && counter == int 3 && !running) {
        return total;
    } else {
        return 0;
    }
}
