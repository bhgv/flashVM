# Our Coco/R Limbo frontend keeps independent checked table reads observable.
implement OddProductFacts; # Match the teaching interface and implementation.
OddProductFacts: module { # Export one scalar entry without sharing table owners.
    value: fn(): big; # Return a full-width big payload through the Dis wrapper.
}; # Finish declaring the interface before defining its implementation.
value(): big { # Modular arithmetic preserves oddness even across overflow.
    samples := table of big {left = big 41, right = big 57}; # Own both inputs locally.
    left := samples.left | big 1; # Force the left operand's lowest bit to one.
    right := samples.right | big 1; # Force the independent right operand odd too.
    product := left * right; # A successful odd-by-odd product has a known low one.
    return (product & big 1) + big 41; # Both raw and optimized Dis return forty-two.
} # Folding the scalar result retains checked reads and their possible failures.
