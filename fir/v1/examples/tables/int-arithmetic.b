implement IntArithmetic;

# This interface selects our Coco/R big/table profile, including typed int arithmetic.
# Both native CLI and in-process C99 source+FIR services compile the same source.
IntArithmetic: module
{
    value: fn(): big;
};


# Compute Unicode string bounds with int operations, then return the numeric slice 42.
# No arguments. Strings remain VM-owned roots throughout arithmetic and slicing.
# Every intermediate int is normalized to signed 32 bits on the NC64 target.
value(): big
{
    # Five Unicode scalars despite the Cyrillic prefix and four-byte emoji.
    text := "Ж42😀z";
    length: int = len text;

    # Division truncates toward zero; remainder has the dividend's sign.
    # Explicit int literals avoid mixing this profile's default big literals.
    width := length / int 2;     # 5 / 2 = 2.
    first := length % int 2;     # 5 % 2 = 1.
    end := first + width;        # Exclusive bound 3.
    last := end - int 1;         # Index 2, the final digit.
    two_windows := width * int 2; # Two windows span four scalars.

    # String indexing consumes the computed int directly, without byte conversion.
    valid := (two_windows == int 4) && (text[last] == int 50);
    if (valid) {
        digits := text[first:end];
        return big digits;
    } else {
        # The unselected arm must neither publish a value nor divide by zero.
        return 1 / 0;
    }
}
