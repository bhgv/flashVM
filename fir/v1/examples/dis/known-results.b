# Compile through our Coco/R frontend and shared FIR optimizer.
implement KnownResults; # Keep implementation and interface names identical.
KnownResults: module { # Declare the bounded scalar teaching entry.
    value: fn(): int; # Return one signed32 result through the usual wrapper.
}; # Finish the interface before defining its body.
value(): int { # Dynamic loop values remain private typed cells.
    sum := 0; # Start the accumulator at zero.
    for (i := 0; i < 8; i++) { # Visit eight values without requiring unrolling.
        zero := (i & 2147483647) >> 31; # A cleared sign bit yields exactly zero.
        minusone := (i | -2147483648) >> 31; # A forced sign bit yields exactly minus one.
        sum += zero - minusone; # Each iteration contributes one.
    } # The sum is eight; producer evaluation remains ordered.
    return sum + 34; # Raw and optimized Dis/SWF must produce forty-two.
} # No grammar, opcode or module ABI change is needed.
