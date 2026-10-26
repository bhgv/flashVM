# Our Coco/R Limbo pipeline keeps table/key owners alive across conditional native calls.
# No pointers cross VM heaps; these are local, typed Bigtable capabilities in the Dis frame.
implement TableDecision;

TableDecision: module {
    value: fn(): big; # Existing capture entry; exactly one return path executes.
};

# Return 42 using snapshot keys and two dynamic decisions, with GC-safe owner maps.
# Each arm terminates: source loops and assignment/join environments are separate work.
value(): big
{
    # Numeric and string keys share one hybrid owner while retaining their distinct identities.
    t := table of big;
    t[0] = big -9223372036854775808;
    t["ключ😀"] = big 42;

    # The immutable key snapshot remains rooted alongside the mutable table owner.
    keys := keysof t;
    answer_key := keys[1];

    # The numeric key is read through its capability; comparison uses all signed 64 bits.
    if (t[keys[0]] < 0) {
        # Both calls use the same table owner across scheduler/collector boundaries.
        if (t[answer_key] == 42) {
            # This is the first static capture slot, and the only executed return here.
            return t[answer_key];
        } else {
            # Conversions in an unselected arm must not be evaluated eagerly.
            return big "not a decimal number";
        }
    } else {
        # This zero divisor is verified/emitted but never executed for the stored MIN value.
        return 1 / 0;
    }
}
