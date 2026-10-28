implement LocalTypes; # Select the complete teaching-profile source module.
LocalTypes: module { # Keep the interface primitive so aliases can live entirely in the body.
    value: fn(): big; # The existing wrapper captures a single big result.
}; # No module alias is required for local type declarations.
value(): big { # Begin the body-local type environment.
    Number: type big; # Declare a sequential alias without allocating a runtime variable.
    Result: type Number; # Bind to the current outer Number before any child shadow exists.
    total := Number 0; # Keep the accumulator in the canonical big representation.
    { # Open a distinct lexical type/value scope.
        Number: type int; # Shadow the outer type only inside this block.
        parsed: Number = Number "42"; # Use checked decimal parsing and signed32 narrowing.
        total = big parsed; # Explicitly widen the local int into the outer accumulator.
    } # Retire the int alias and restore the outer big Number.
    { # Reuse the spelling in a sibling scope with a different canonical type.
        Number: type string; # This alias must not inherit the previous block's int cache.
        text: Number = Number total; # Format the accumulated big value as a string.
        total = big text; # Parse it back while the string owner remains rooted.
    } # Retire the string alias and its local value binding.
    for (i := big 0; i < 2; i++) { # Exercise type retirement across compiled loop backedges.
        Step: type big; # Compile this local declaration once, regardless of runtime iteration count.
        total += Step 0; # Preserve 42 while using the loop body's canonical alias.
    } # Step is unavailable to later source statements.
    return Result total; # Result still means the outer big type and reports exactly 42.
} # All local type state is discarded before the next compiler request.
