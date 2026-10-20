# Numeric value() adapter: the declared module has no external dependencies.
implement V;
V: module {
    value: fn(): int; # Export the single tutorial entry point.
};
value(): int {
    x: int = 6; # Initialize a signed i32 binding.
    y: int = x * 7; # Arithmetic wraps at 32 bits; this result is 42.
    x = y - x; # Rebinding x does not change y; x becomes 36.
    return x; # The tutorial SWF harness traces the returned number.
}
