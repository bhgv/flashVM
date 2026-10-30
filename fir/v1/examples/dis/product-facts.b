# Compile through our Coco/R Limbo frontend; use the big-capable Dis profile.
implement ProductFacts; # Keep the implementation and interface names identical.
ProductFacts: module { # Export the scalar teaching entry used by CLI and SDK tests.
    value: fn(): big; # Return a full-width big payload, not a narrowed host integer.
}; # Close the interface before defining its implementation.
value(): big { # Own all table values within this invocation; no shared native pointers.
    samples := table of big; # Allocate a checked typed table, keeping reads dynamic.
    samples["left"] = big 41; # Supply a nonzero left operand for the raw execution.
    samples["right"] = big 57; # Supply an independent right operand.
    left := samples["left"] & big -8; # Every successful left value is divisible by eight.
    right := samples["right"] & big -8; # The right value is independently divisible by eight.
    product := left * right; # Modular multiplication makes six low result bits zero.
    return (product & big 63) + big 42; # Known-bit folding produces42 without removing read errors.
} # The raw and optimized Dis implementations must agree, including eager failures.
