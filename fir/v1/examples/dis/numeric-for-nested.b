# Nested counted loops exercise independent staged streams and lexical cells.
implement NumericForNested; # Match the module declaration below.
NumericForNested: module { # Describe the bounded scalar tutorial entry.
    value: fn(): int; # Use signed32 wrapping arithmetic throughout.
}; # End the signature.
value(): int { # Return one result after both counted loops finish.
    total := 0; # Retain an accumulator outside either loop-header scope.
    i := 42; # Keep an outer name that the first header will shadow.
    for (i := 0; i < 3; i++) { # The header's i stays separate from outer i.
        for (j := 7; j > 0; j--) { # Each pass constructs its own inner loop cells.
            i := 2; # Shadow the header i only inside this inner body block.
            total += i; # Add two exactly twenty-one times to reach forty-two.
        } # Run j-- on backedges; retire j and its body-local i on exit.
    } # The outer header step still updates its own i, not either shadow.
    if (i != 42) { # Prove the outer binding survived both header scopes.
        return -7; # Expose a scope leak as an independently captured wrong result.
    } # Fall through when the original outer i remains intact.
    return total; # Capture the expected forty-two after normal loop completion.
} # End the nested-loop tutorial.
