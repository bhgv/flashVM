# Our Coco/R source compiler emits these exact typed integer expressions.
implement IntegerPairs; # Keep the interface and implementation names equal.
IntegerPairs: module { # Declare one bounded tutorial entry, not a new call ABI.
    value: fn(): int; # Return the existing signed32 scalar result.
}; # Complete the interface before defining the entry.
value(): int { # Own a loop counter and accumulator in private typed cells.
    sum := 0; # Start the running sum at zero.
    for (i := 0; i < 4; i++) { # Keep i dynamic over the loop back edge.
        canceled := (i + 7) - 7; # Wrapping addition/subtraction cancels exactly.
        unmasked := (i ^ 3) ^ 3; # Applying the same XOR mask twice restores i.
        absorbed := i & (i | 255); # Bit absorption restores i without range inference.
        sum += canceled + unmasked + absorbed; # Add three copies of i each time.
    } # The four iterations contribute eighteen in total.
    return sum + 24; # Both raw/optimized Dis and SWF must produce forty-two.
} # End the entry; floating arithmetic does not share these identities.
