# Compile with our Coco/R Limbo frontend and the Dis backend.
implement BigNegFacts; # Match the teaching interface below.
BigNegFacts: module { # Use only the existing typed table extension.
    value: fn(): big; # Return an exact64-bit payload through the usual wrapper.
}; # Finish the interface before defining the body.
value(): big { # Keep the fetched value dynamic to the SSA bit analysis.
    samples := table of big; # Own one typed table in this module invocation.
    samples["sample"] = big 41; # Store an exact payload with checked table semantics.
    odd := samples["sample"] | big 1; # The fetched value may fail, but its low bit is forced.
    negative := -odd; # Modular negation preserves that known low one.
    return (negative & big 1) + big 41; # Both raw and optimized Dis return forty-two.
} # Table evaluation/errors remain observable when the final result folds.
