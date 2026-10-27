implement UnicodeStrings;

# The big/table result profile includes typed string primitives in our Coco/R frontend.
# CLI and C99 source+FIR clients compile this module without the Limbo bootstrap.
UnicodeStrings: module
{
    value: fn(): big;
};


# Return 42 after slicing and indexing Unicode strings without changing aliases.
# No arguments. String results belong to this VM; frame root maps retain them
# through one-instruction scheduler/GC slices and checked conversion calls.
value(): big
{
    # Seven Unicode scalars: A, é, Ж, emoji, e, combining acute accent, Z.
    # The visible accented e is two scalars; no grapheme normalization is performed.
    text := "AéЖ😀éZ";
    original := text;

    # String bounds are int and the upper bound is exclusive.
    # This slice contains é, Ж and emoji, regardless of their UTF-8 byte widths.
    part := text[int 1:int 4];

    # Length and indexed character are int, matching Inferno string operations.
    # Emoji is U+1F600 (128512), one scalar rather than two UTF-16 code units.
    valid := (len part == int 3) && (part[int 2] == int 128512);
    unchanged := (len original == int 7) && (original[int 0] == int 65);

    if (valid && unchanged) {
        # The numeric conversion must see exactly the sliced bytes, not the y suffix.
        # Neither original nor number_text is modified by creating a slice.
        number_text := "x42y";
        digits := number_text[int 1:int 3];
        return big digits;
    } else {
        # Wrong Unicode geometry or alias handling cannot yield a successful result.
        return 1 / 0;
    }
}
