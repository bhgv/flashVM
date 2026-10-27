implement Constructors;

# The big result selects our Coco/R hybrid-table extension profile.
# The same source is compiled by CLI and the IDE-style in-process C99 services.
Constructors: module
{
    value: fn(): big;
};


# Build editor-style scalar settings and a numeric prefix, then return its sum 42.
# No arguments. Table aliases share one owner; numeric slices create independent copies.
# Every constructor value must be big; strings below are keys rather than stored values.
value(): big
{
    # Positional fields occupy indexes 0 and 1. Named fields use string keys.
    # An explicit numeric key does not advance the positional counter or fill gaps.
    settings := table of big {
        40,                        # Numeric index 0.
        2,                         # Numeric index 1.
        font_size = 11,            # Same key as settings["font_size"].
        line_numbers = 1,          # A scalar flag, not a new bool container type.
        ["gutter-width"] = 4,      # Brackets allow punctuation and Unicode keys.
        [4] = 99,                  # A sparse numeric entry; indexes 2 and 3 are absent.
    };                             # One trailing comma is allowed.

    # len counts only the contiguous numeric prefix: two entries, despite six keys.
    # keysof captures all six identities, including named and sparse numeric entries.
    keys := keysof settings;
    numbers := settings[0:2];       # Independent numeric slice containing 40 and 2.

    # Table aliases are mutable shared owners. This changes settings[0], but cannot
    # change the previously copied numeric slice used for the final result.
    alias := settings;
    alias[0] = 999;

    # Constructor fields remain ordinary typed accesses after lowering to FIR.
    valid := (len settings == 2) && (len keys == 6) &&
             (settings.font_size == 11) && (settings.line_numbers == 1);
    if (valid) {
        return numbers[0] + numbers[1]; # The slice still yields 42.
    } else {
        return 0;                      # A distinct result if any invariant fails.
    }
}
