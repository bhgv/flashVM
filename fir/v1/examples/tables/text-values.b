implement TextValues;

# A big result selects our Coco/R Limbo table/string/int extension profile.
# CLI tools and in-process C99 source/FIR services compile this same module.
TextValues: module
{
    value: fn(): big;
};


# Parse editor-style text fields and a retained table key, then compare filenames.
# No arguments. Strings and key snapshots stay rooted in the owning Dis VM.
# Return 42 only when all checks succeed; the false arm returns zero.
value(): big
{
    # A settings table uses a decimal text key rather than a numeric index.
    settings := table of big;
    settings["42"] = 1;
    keys := keysof settings;       # Own an immutable snapshot of the key identities.
    delete settings["42"];        # The snapshot remains usable after table mutation.
    line := int keys[0];           # Complete decimal big parse, then signed32 narrowing.

    # Bounds count Unicode scalars, including the Cyrillic prefix and one emoji.
    # The borrowed slice need not end in NUL in the independent reference runner.
    field := "Ж42😀";
    digits := field[int 1:int 3];
    parsed := int digits;          # Exact parse of the entire two-character slice.

    # String order is lexicographic by Unicode scalar, not numeric/natural order.
    # In particular, file10 sorts before file2 because '1' is smaller than '2'.
    earlier := "file10";
    later := "file2";
    ordered := earlier < later;

    # All comparisons return bool; && evaluates later checks only when needed.
    # Reflection observes the original string key kind, unchanged by int conversion.
    valid := ordered && (line == parsed) && (typeof keys[0] == "string");
    if (valid) {
        return big parsed;        # Exact widening, without floating conversion.
    } else {
        return 0;                 # SDK acceptance distinguishes a failed invariant.
    }
}
