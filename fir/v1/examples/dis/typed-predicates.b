implement TypedPredicates; # Select the Limbo source module compiled through Coco/R.
TypedPredicates: module { # Declare one exported function for the teaching harness.
    value: fn(): big; # A big return selects the profile with typed tables and bool values.
}; # Bool locals are inferred from comparisons rather than a declaration keyword.
value(): big { # The temporary table belongs to this function's runtime execution.
    Number: type int; # Canonicalize the local alias to exact signed32.
    bounds := table of big {low = big -1, high = big 42}; # Store two values for real runtime loads.
    low: Number = Number bounds.low; # Explicitly narrow the first loaded operand once.
    high: Number = Number bounds.high; # Explicitly narrow the second operand once.
    ascending := low < high; # Emit i32.lt.signed directly without widening to big.
    different := low != high; # Emit i32.eq followed by normalized bool.not.
    consistent := ascending == different; # Compare inferred bools with bool.eq.
    stable := !!consistent; # Preserve the bool; optimization removes double negation.
    accepted := stable && !(low >= high); # Keep lazy RHS evaluation and strict bool typing.
    return (big accepted) * big 42; # Explicitly convert the final bool; capture exactly 42.
} # GC releases the local owner in both native and interpreter dispatch.
