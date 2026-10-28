implement RealConditions; # Select the Limbo source module compiled through Coco/R.
RealConditions: module { # Export one homogeneous real teaching function.
    value: fn(): real; # Keep the interface and implementation result types identical.
}; # No int/real coercion or external module ABI is implied by this interface.
value(): real { # Terminal arms below each return one binary64 result.
    Measurement: type real; # Canonicalize a local type alias without runtime storage.
    sample: Measurement = Measurement 2.5; # Evaluate the measured value once as real.
    limit: Measurement = Measurement 3.0; # Keep the second comparison operand the same type.
    if (sample <= limit) { # Use ordered f64.le; NaN would select the other arm.
        Scale: type real; # Give this arm its own sequential local alias scope.
        factor: Scale = Scale 17.0; # Explicit alias spelling still denotes binary64.
        return sample * factor; # Capture exactly 42.5 in interpreter, JIT and linked SWF.
    } else { # Only the selected arm publishes its own capture-validity flag.
        return -0.0; # Preserve the binary64 fallback's sign when this arm is selected.
    } # Branch scopes retire independently before compilation finishes.
} # No mutable owner, cross-heap pointer or channel is created by this example.
