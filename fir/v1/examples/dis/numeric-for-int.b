# Compile with fir1_source limbo; value() is the bounded tutorial entry.
implement NumericForInt; # Match the declared module without an external loader.
NumericForInt: module { # Expose the single numeric entry to the source compiler.
    value: fn(): int; # Keep every local and arithmetic operand signed32.
}; # Close the module signature.
value(): int { # Return one captured scalar result to the harness.
    sum := 0; # Promote this outer accumulator into a private FIR i32 cell.
    for (i := 0; i < 10; i++) { # Initialize once, test each pass, stage postfix step.
        if (i == 3) { # Skip exactly one summand using a grouped bool condition.
            continue; # Execute i++ before testing the next iteration.
        } # Rejoin the ordinary body path when i differs from three.
        if (i == 8) { # Stop before adding eight or running another header step.
            break; # Jump directly to the counted-loop exit.
        } # Fall through for the remaining indices.
        sum += i; # Sum 0, 1, 2, 4, 5, 6, 7 to produce twenty-five.
    } # Retire the loop-header binding i; its name cannot escape this scope.
    sum += 17; # Continue after the loop and reach the documented result.
    return sum; # Capture exactly forty-two with its executed flag.
} # End the entry; no implicit return or full function-call ABI is introduced.
