implement LeasedTables;

# A complete module for our Coco/R big/table profile, shared by CLI and C99 API.
LeasedTables: module
{
    value: fn(): big; # No arguments; returns the exact signed-big value 42.
};


# Demonstrate automatically scoped table mutation, without manual native imports.
# Owners stay local to this VM; GC retires them after their last program reference.
# A runtime exception releases retained ownership before handler lookup, but does
# not undo completed mutations. This source profile has no channel/spawn syntax.
value(): big
{
    # The constructor is private until binding; its fields need no retained scope.
    document := table of big {20, 2, label=40};
    alias := document;

    # One destination owner is acquired before key evaluation and held through
    # both RHS reads. Recursive reads through either alias use that same owner.
    alias[0] = document[0] + alias[0];

    # The negative key resolves while ownership is retained: -1 is index 1 here.
    # A compound operation captures the old 2 before adding the first element 40.
    alias[-1] += document[0];

    # Snapshot keys retain their own immutable identities, not native addresses.
    # The copied key selects the same field while the destination table is held.
    keys := keysof document;
    document[keys[2]] += 2;

    # Capture the full dense range, copy it and splice that copy back under the
    # destination lease. Text fields remain outside the replaced dense prefix.
    document[0:-1] = alias[0:-1];

    # The sequence remains [40,42] and label is 42. This returns one exact big.
    return document[-1] + document.label - 42;
}
