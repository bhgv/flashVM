# Our Coco/R source compiler emits this bounded int entry without the bootstrap.
implement IntegerIdentities; # Match the module declared directly below.
IntegerIdentities: module { # Describe the one supported tutorial entry.
    value: fn(): int; # Return a signed32 scalar through the existing backend.
}; # Finish the module interface without importing native frames.
value(): int { # Begin the entry whose loops keep operands dynamic in FIR.
    sum := 0; # Keep the running total in a private signed32 cell.
    for (i := 0; i < 4; i++) { # Visit zero through three using the existing CFG.
        zero := (i * 0) + (i % 1) + (i ^ i); # Three total rules leave zero and keep producers.
        same := (i / 1) + (i << 0) + (i & -1); # Three rules retain one copy of i each.
        sum += zero + same; # Keep three copies of i in the accumulator.
    } # The six terms sum to eighteen over the four iterations.
    return sum + 24; # Produce forty-two in raw/optimized Dis and optimized SWF.
} # End the entry; this does not add a general function-call ABI.
