# Our Coco/R source compiler emits typed FIR without a bootstrap compiler.
implement IntDivmodSwf; # Match the implementation and module interface names.
IntDivmodSwf: module { # Declare the bounded signed32 executable profile.
    value: fn(): int; # Return one scalar through either ordinary target backend.
}; # End the interface; this example imports no runtime library.
value(): int { # Begin with private loop and accumulator cells.
    sum := 0; # Start the running total at zero.
    for (i := -7; i < 8; i++) { # Visit both signs and zero with a dynamic numerator.
        quotient := i / 3; # The proven nonzero divisor permits ordinary SWF lowering.
        remainder := i % 3; # Remainder has the numerator's sign, as in FIR and Dis.
        sum += quotient * 3 + remainder; # Reconstruct i exactly at every iteration.
    } # The symmetric range sums to zero without folding the whole loop.
    return sum + 42; # Raw/optimized Dis and linked SWF must return forty-two.
} # Finish; unknown divisors still require a backend that preserves their errors.
