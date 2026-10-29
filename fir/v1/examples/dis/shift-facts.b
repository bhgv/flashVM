# Our Coco/R frontend preserves these checked operations in typed FIR.
implement ShiftFacts; # Match the interface below.
ShiftFacts: module { # Declare the existing bounded teaching entry.
    value: fn(): int; # Return one signed32 payload through the usual wrapper.
}; # Finish the declaration before its implementation.
value(): int { # Keep i dynamic over the loop back edge.
    sum := 0; # Initialize the accumulator.
    for (i := 0; i < 8; i++) { # Visit eight values without static unrolling.
        sum += (1 << ((i & 127) >> 2)) + i / ((i | 1) << 1); # Derive a bounded count and nonzero divisor by shifts.
    } # Four ones and four twos contribute twelve; every quotient is zero.
    return sum + 30; # Raw and optimized Dis/SWF must return forty-two.
} # No new opcode, parser syntax or module ABI is introduced.
