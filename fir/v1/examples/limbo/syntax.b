# Native Limbo radix and inferred declaration example; only the i32 profile lowers.
implement V;


V: module
{
    value: fn(): int;  # The adapter recognizes this explicit tutorial contract.
};


value(): int
{
    x := 2r1010;     # Base two digits yield ten; := infers the numeric binding.
    y: int = 16r20;  # Base sixteen digits yield thirty-two.
    x += y;          # The mutable binding now contains forty-two.
    x *= -1;         # Signed wrapping arithmetic produces minus forty-two.
    return -(x);     # General unary negation restores forty-two for the trace harness.
}
