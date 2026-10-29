# This entry uses our Coco/R Limbo compiler and ordinary FIR control flow.
implement IntegerComparisons; # Keep the implementation and interface names equal.
IntegerComparisons: module { # Declare the small executable source profile.
    value: fn(): int; # Return one signed32 value through the existing toolchain.
}; # Close the interface; no new calling convention is introduced.
value(): int { # Own the local cells used by this deterministic example.
    sum := 0; # Begin the accumulator at zero.
    for (i := 0; i < 4; i++) { # Keep i dynamic across the loop back edge.
        if (i == i) { # Integer equality of one evaluated SSA value is true.
            sum += i; # Add zero, one, two and three, giving six.
        } # End the always-selected branch after optimization.
        if (i < i) { # Strict integer ordering of the same value is false.
            return 0; # This branch cannot run and the optimizer can remove it.
        } # End the reflexive less-than check.
        if (i < (-2147483647 - 1)) { # No signed32 value lies below its minimum.
            return 1; # This branch must remain unreachable at every loop value.
        } # End the lower-bound check.
        if (2147483647 < i) { # No signed32 value lies above its maximum.
            return 2; # This branch is also unreachable without a range analysis.
        } # End the upper-bound check.
    } # The loop still executes normally and contributes six.
    return sum + 36; # Raw and optimized Dis/SWF must both produce forty-two.
} # Finish the entry; floating NaN uses different comparison rules.
