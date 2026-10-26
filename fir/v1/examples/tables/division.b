# Exact signed division in our bounded Coco/R big-result profile.
# Table reads keep the operands dynamic for the interpreter and native JIT.
implement DivisionExample;


DivisionExample: module
{
    value: fn(): big; # Capture one exact big result without importing Sys.
};


# Demonstrate signed minimum/-1, truncation toward zero and remainder identity.
# Owners remain local to this VM; each read supplies one exact signed 64-bit value.
value(): big
{
    inputs := table of big; # The frame GC map retains this mutable table owner.
    inputs.minimum = big -9223372036854775808; # Exact signed minimum, not an AS Number.
    inputs.minus_one = big -1; # A runtime-loaded divisor prevents constant folding.
    inputs.negative = big -17; # Ordinary signed arithmetic also stays dynamic.
    inputs.positive = big 5; # A positive divisor makes the remainder sign visible.

    wrapped := inputs.minimum / inputs.minus_one; # Defined VM result: signed MIN again.
    edge_remainder := inputs.minimum % inputs.minus_one; # The paired remainder is zero.
    quotient := inputs.negative / inputs.positive; # Truncate toward zero: -3, not -4.
    remainder := inputs.negative % inputs.positive; # The dividend's remainder sign: -2.

    # The comparison also reads a table value, selecting exactly one source CFG arm.
    if (wrapped == inputs.minimum) {
        # Reconstruct -17 from -3*5 + -2, then subtract the original: that part is zero.
        # Negating quotient and multiplying by 14 supplies the final exact result 42.
        return -quotient * 14 + edge_remainder +
               (quotient * inputs.positive + remainder - inputs.negative);
    } else {
        # An unselected zero-divisor arm must stay unevaluated, before and after opt.
        return 1 / 0;
    }
}
