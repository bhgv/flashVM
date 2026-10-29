# Compile with our Coco/R Limbo frontend, then use either Dis or SWF8.
implement IntegerFacts; # Match the interface name below.
IntegerFacts: module { # Declare the bounded tutorial interface.
    value: fn(): int; # Return one signed32 result through the existing wrapper.
}; # End the interface before the implementation.
value(): int { # Keep the loop counter dynamic across its back edge.
    sum := 0; # Start the accumulator at zero.
    for (i := 0; i < 4; i++) { # Visit zero, one, two and three.
        shifted := 1 << (i & 31); # Known zero upper bits prove a legal shift count.
        quotient := i / (i | 1); # A known set bit proves a nonzero dynamic divisor.
        sum += shifted + quotient; # Add seventeen over the four iterations.
    } # Producers still execute; the proof does not suppress their errors.
    return sum + 25; # Raw and optimized Dis/SWF must produce forty-two.
} # End the entry without adding a new call or container ABI.
