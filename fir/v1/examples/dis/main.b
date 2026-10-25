# Our numeric Limbo profile: this file is parsed by Coco/R, not by the temporary Inferno compiler.
implement DisAnswer;


DisAnswer: module
{
    value: fn(): int; # The existing tutorial adapter lowers this result to FIR @main's trace capture.
};


# Return one signed i32 result, demonstrating wrap at each intermediate operation.
# No parameters, imports, heap owners or channels are involved in this deliberately bounded profile.
value(): int
{
    maximum := 2147483647; # Largest positive signed i32.
    wrapped := maximum + 1; # Wrap immediately to -2147483648, despite the target's wider native word.
    product := wrapped * -1; # Wrap again to -2147483648; do not keep a wider temporary value.
    return product - 1; # Final wrap yields 2147483647, captured in module-data cell zero.
}
