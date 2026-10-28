implement NumericFlowInt; # Select the exact signed32 teaching profile.
NumericFlowInt: module { # Export only the existing bounded numeric entry point.
    value: fn(): int; # All arithmetic and local updates below remain signed32.
}; # Bool comparison results remain internal to control-flow expressions.
value(): int { # Every reachable exit returns an int without implicit coercions.
    sum := 0; # This initialized value is promoted into a private i32 cell.
    index := 0; # The loop carries a separate nonaliasing scalar cell.
    while (index < 9) { # Load the current index before selecting body or exit.
        index += 1; # Commit the step before either structured transfer.
        if (index == 3) { # A missing else preserves the ordinary false continuation.
            continue; # Skip only this iteration and return to the innermost test.
        } # The surviving false edge rejoins the loop body.
        if (index == 9) { # Do not add the terminal index to the accumulated sum.
            break; # Exit only the innermost loop, retaining earlier cell writes.
        } # Values 1, 2, 4, 5, 6, 7 and 8 reach the addition below.
        sum += index; # The final accumulated value is exactly 33.
    } # Source-local initialization facts remain conservative after a potentially empty loop.
    if (sum == 33) { # Demonstrate a nonterminal numeric join after the loop.
        sum += 9; # Update the selected path to exactly 42.
    } else { # A failed oracle has a real early function return.
        return -7; # This capture remains invalid on the demonstrated successful path.
    } # The only open path now reaches the final return.
    return sum; # Capture 42 after all loop and branch state has been read through cells.
} # No heap owner, shared pointer, new Dis opcode or generic function call is needed.
