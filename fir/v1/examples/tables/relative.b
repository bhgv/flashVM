implement Relative;

# Select our Coco/R table/string profile; values in a table remain big.
Relative: module
{
    value: fn(): big;
};


# Read from the dense prefix's end and make independent copies before mutation.
# No arguments. The source tables/strings and copied values remain VM-owned roots.
# Return 42 on success; the unselected arm returns zero for SDK trace checks.
value(): big
{
    source := table of big {40, 2, label = 99, [100] = 123};
    last := source[-1];            # 2: sparse key 100 and label do not affect len.
    copied := source[0:-1];        # Copy numeric indexes 0 and 1, without dictionary keys.
    source[-1] = 7;                # Change the original last element only.

    text := "Ж42😀";
    digits := text[int -3:int -2]; # Negative right bound includes that character: "42".
    whole := text[int 0:int -1];   # Full Unicode value copy, four scalars.

    valid := (last == 2) && (len copied == 2) && (countof copied == 2) &&
             (big digits == 42) && (len whole == int 4);
    if (valid) {
        return copied[-2] + copied[-1]; # Still 40 + 2 after source mutation.
    } else {
        return 0;
    }
}
