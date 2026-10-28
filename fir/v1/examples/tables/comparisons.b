implement ComparisonValues;

# This bounded interface selects our exact big/table Coco/R source profile.
# It is translated directly to FIR, then to Dis, without the Limbo bootstrap.
ComparisonValues: module
{
    value: fn(): big;
};


# Return 42 after evaluating first-class comparison values and logical negation.
# No arguments. Local owners stay in this VM; the function exports only a big.
# Comparison operands are evaluated once in authored left-to-right order.
value(): big
{
    # A table read keeps the comparison live when FIR optimization is enabled.
    # These adjacent values exceed binary64's consecutive-integer range.
    document := table of big;
    document.low = big 9007199254740993;
    document.high = big 9007199254740994;

    # All six relations return a normalized bool. They no longer require an if.
    ascending := document.low < document.high;
    descending := document.low >= document.high;

    # Logical negation accepts bool, without implicit numeric/string truthiness.
    # Equality between two bools is supported; ordered bool comparison is not.
    consistent := ascending == !descending;

    # Explicit int casts remain typed: both operands have the same static type.
    # The comparison uses signed32 directly and never routes through float.
    signed_order := int -2147483648 < int 2147483647;

    # Membership is also bool and can be negated or compared with another bool.
    missing := !("absent" in document);
    ready := consistent == missing;

    if (ready) {
        # Inferred bool values are explicitly converted when used as numbers.
        # The existing branch-local scope keeps answer local to this arm.
        answer := big signed_order + 41;
        return answer;
    } else {
        # An unselected return does not publish a capture or execute bad reads.
        return document.absent;
    }
}
