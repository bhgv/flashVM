# Compile this source with our Coco/R frontend; it is not a bootstrap-only example.
implement SparseFacts;

# The ordinary FIR/Dis backend exports one scalar result for the host to inspect.
SparseFacts: module
{
    value: fn(): int; # Return42 after visiting dynamic signed source values.
};


# Keep input dynamic across loop iterations, normalize it to0..7, then multiply
# by3 (two set bits). Its product is0..21: every bit above bit4 must be zero.
# The upper projection contributes zero; the lower projection sums to84.
# Subtract42 to expose a stable result without changing modular integer rules.
value(): int
{
    total := 0;
    for (index := 0; index < 8; index++) {
        input := index - 4;
        product := (input & 7) * 3;
        high := product & ~31;
        total += (product & 31) + high;
    }
    return total - 42;
}
