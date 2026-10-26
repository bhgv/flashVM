# Our Coco/R Limbo table profile: all-entry count, membership and captured deletion.
# Values remain exact big; logical results have explicit big/int conversions.
implement MutationExample;


MutationExample: module
{
    value: fn(): big; # Return one selected result through the normal capture harness.
};


# Keep local table/snapshot owners rooted while mutations flow through FIR to Dis.
# No native pointer or implicit remote projection is exported by this tutorial.
value(): big
{
    entries := table of big; # One mutable owner, shared by the alias below.
    entries[0] = big 0; # Zero is a present value, distinct from a missing key.
    entries[1] = big 11; # This index must survive deletion of index zero unchanged.
    entries.answer = big 42; # Dotted syntax is the literal string key "answer".
    alias := entries; # Copy the owner reference; changes remain visible through either name.
    snapshot := keysof entries; # Independent immutable keys: 0, 1 and "answer".

    zero_exists := 0 in entries; # Test presence without loading/raising for absence.
    removed := delete alias[snapshot[0]]; # Delete the actual snapshot key, not its ordinal.
    removed_again := delete entries[0]; # Already absent: false, with no implicit insertion.
    delete entries.missing; # Statement form keeps its mutation/error effect even if unused.
    stale_key_exists := snapshot[0] in entries; # The retained key is valid but now absent.

    if ("answer" in entries) {
        # countof counts both remaining pairs; len sees the hole at numeric index zero.
        # The snapshot still has three keys, and membership/deletion normalize to 0/1.
        return entries.answer + (countof entries - 2) + len entries + (len snapshot - 3)
               + (big zero_exists - 1) + (big removed - 1)
               + big removed_again + big stale_key_exists; # Exactly 42.
    } else {
        return entries.must_not_be_loaded; # A missing read would raise if the wrong arm ran.
    }
}
