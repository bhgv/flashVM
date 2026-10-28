implement GroupedConditions; # Select the module for our Coco/R numeric adapter.
GroupedConditions: module { # Export a single typed function without an external ABI.
    value: fn(): real; # Keep the result and all arithmetic operands binary64.
}; # Intermediate bools below cannot become numeric locals or return values.
value(): real { # A terminal decision still returns on every execution path.
    Measure: type real; # Resolve this lexical type alias before its uses.
    sample: Measure = Measure 2.5; # Evaluate the sample once into a numeric SSA value.
    lower := 0.0; # This lower boundary belongs to the same homogeneous profile.
    upper := 3.0; # The two boundaries define a nonempty half-open interval.
    if (((sample < lower) == (sample >= upper)) && !!(sample == sample)) { # Compare predicates and reject NaN through repeated unary negation.
        return ((sample + 0.0) * 17.0); # Arithmetic parentheses use the same grammar and produce exactly 42.5.
    } else { # A sample outside the interval or an unordered sample takes this arm.
        return -0.0; # Preserve the fallback sign without converting bool into a number.
    } # Frame-local bool merge cells retire without retaining GC owners.
} # The same module runs through Dis interpreter/JIT and linked SWF.
