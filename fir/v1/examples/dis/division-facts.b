# Compile with our Coco/R Limbo frontend; no native bootstrap extension is needed.
implement DivisionFacts;

DivisionFacts: module
{
    value: fn(): int; # Return1344, captured by the ordinary FIR/Dis backend.
};


# The loop keeps its input dynamic across iterations. A positive division by8
# bounds (input&255)/8 to0..31, so the checked shift is valid for every input.
# The second term is signed: aligned negative integers have remainder0 modulo8.
# Accumulate42*32 and43*32, then subtract43*32 to expose1344 as one scalar result.
# No callbacks, external modules, pointers or frame-synchronous graphics are used.
value(): int
{
    total := 0;
    for (index := 0; index < 2; index++) {
        input := 42 + index;
        total += input << ((input & 255) / 8);
        total += ((index - 9) & -8) % 8;
    }
    return total - 1376;
}
