# Our Coco/R Limbo big profile: exact comparisons and a fully terminating decision tree.
# No bootstrap compiler, hidden floating conversion or table service is needed.
implement BigDecision;

BigDecision: module {
    value: fn(): big; # Existing tutorial entry; its return is captured by generated Dis.
};

# Return MIN exactly. Both arms must terminate; declarations/assignments belong in the prelude.
# This restriction avoids inventing a phi node or merging different local binding environments.
value(): big
{
    # These explicit same-type casts preserve exact bits and can be optimized as copies.
    a := big -9223372036854775808;
    b := big -1;

    # Comparison is signed over all 64 bits; MIN must compare below -1.
    if (a < b) {
        # A second comparison checks exact equality independently of arithmetic precision.
        if (a == -9223372036854775808) {
            # The specified MIN/-1 result wraps to MIN without trapping the host CPU.
            return a / b;
        } else {
            # The compiler verifies/emits this arm, but execution never selects it here.
            return 0;
        }
    } else {
        # The third static trace slot remains unexecuted in the original Dis image.
        return 7;
    }
}
