# Our Coco/R frontend lowers this numeric program to ordinary typed FIR.
implement IntShiftsSwf; # Match the implementation to the interface below.
IntShiftsSwf: module { # Declare the bounded executable source profile.
    value: fn(): int; # Return a signed32 scalar; no runtime imports are needed.
}; # End the public interface.
value(): int { # Own the accumulator and loop counter in private scalar cells.
    sum := 0; # Begin at zero before visiting four dynamic inputs.
    for (i := 0; i < 4; i++) { # Keep i dynamic through the loop back edge.
        left := i << 2; # A proven count of two is valid in FIR and AVM1.
        right := -8 >> 2; # Arithmetic right shift of negative eight produces -2.
        sum += left + right; # Add -2, 2, 6 and 10, for a total of sixteen.
    } # Keep the source loop; this is not only constant folding.
    return sum + 26; # Raw and optimized SWF and Dis must produce forty-two.
} # Finish the entry without extending SWF's masked-count semantics.
