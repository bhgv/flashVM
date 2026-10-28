# Compile with the same source API/CLI used by the portable IDE SDK.
implement NumericForReal; # Supply a module name checked against its declaration.
NumericForReal: module { # Declare the bounded homogeneous real entry.
    value: fn(): real; # Select binary64 expressions and private f64 cells.
}; # Close the signature without a general module loader contract.
value(): real { # Execute with either Dis interpreter/JIT or linked AVM1 SWF.
    sum := 0.0; # Preserve an exact binary64 accumulator across backedges.
    for (i := 0.0; i < 7.0; i++) { # Add exact one at the deferred header step.
        if (i == 2.0) { # Demonstrate that a skipped body still advances i.
            continue; # Reach the staged postfix update, then recheck the test.
        } # The other six passes contribute to the accumulator.
        sum += 7.0; # Produce forty-two from six additions without mixed casts.
    } # Header/body scopes end; no i binding leaks into the continuation.
    sum += 0.5; # Continue with ordinary real arithmetic after the loop.
    return sum; # Capture exact binary64 forty-two and one half.
} # End the complete numeric tutorial.
