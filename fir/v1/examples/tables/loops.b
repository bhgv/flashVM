implement LoopTables;

# The CLI and C99 source API use this same Coco/R big/table profile.
LoopTables: module
{
    value: fn(): big; # No arguments; return the checked exact big result 42.
};


# Build a table with while, then traverse its captured keys with for-in.
# Owners remain local to this VM. Runtime GC retains handles across backedges;
# each table write retains ownership only for that assignment, not the full loop.
value(): big
{
    values := table of big;
    index := big 0;
    sum := big 0;

    # Initialized outer big locals become private frame cells before the test.
    # The condition reads the current index on every pass, including the first.
    while (index < 7) {
        values[index] = index;
        sum += values[index];
        index += 1;
    }

    # The key snapshot is captured once. key is a snapshot-owned key capability,
    # and value is copied from the live table before entering each body iteration.
    # Both names are lexical locals and disappear when the traversal finishes.
    visited := big 0;
    for (key, value in values) {
        visited += value;

        # Mutating existing values does not change this traversal's captured keys.
        # Aliases and retained keys still refer to the original mutable owner.
        values[key] = value * 2;
    }

    # Both accumulators equal 0+1+...+6 = 21. The table now contains doubled values.
    # Terminal branches retain the existing two-trace executable harness layout.
    if (sum + visited == 42) {
        return sum + visited;
    } else {
        return 0;
    }
}
