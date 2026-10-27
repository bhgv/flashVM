implement Splice;

Splice: module
{
    value: fn(): big; # Deterministic tutorial result, consumed by the FIR trace harness.
};


# Demonstrate resized table writes and Unicode rune writes through our Coco/R path.
# No arguments. Tables share mutable owner identity; strings retain value semantics.
# Runtime bounds/allocation errors raise before publishing a partially changed value.
value(): big
{
    # The text key is outside the sequence and survives every prefix replacement.
    t := table of big {10, 20, 30, 40, label = 42};
    alias := t;

    # Positive end is exclusive: replace 20 and 30 with one value.
    # The old tail shifts left, giving [10, 2, 40].
    t[1:3] = table of big {2};

    # An empty destination range inserts. RHS aliases the same table, so the
    # runtime reads the original prefix before publishing the new contents.
    # The result is [10, 10, 2, 40, 2, 40], visible through alias as well.
    t[1:1] = t;

    # Rune positions count A, Ж, 😀 and Z as four elements, despite UTF-8 width.
    # Negative end is inclusive: -2 selects 😀, so this replaces Ж and 😀.
    text := "AЖ😀Z";
    original := text;
    text[int 1:int -2] = "é";

    # Scalar assignment requires an explicit int Unicode scalar. -1 addresses
    # the final rune. text becomes Aé!, while original remains AЖ😀Z.
    text[int -1] = int 33;

    # Check the dictionary survived storage replacement through the same owner.
    # 6 + 2 + 33 + 4 - 3 = 42 observes table aliasing and rune lengths as well.
    if (alias.label == 42) {
        return len t + alias[4] + big text[int -1] + big len original - 3;
    } else {
        # The SDK checks both trace slots and confirms this arm did not execute.
        return 0;
    }
}
