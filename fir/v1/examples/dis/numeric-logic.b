implement NumericLogic; # Compile this module through our Coco/R Limbo source library or CLI.
NumericLogic: module { # Keep the interface independent of the host renderer.
    value: fn(): real; # Return one homogeneous binary64 result for this teaching profile.
}; # The example needs no external module ABI or native owner pointer.
value(): real { # All source bindings and arithmetic below remain real.
    sample := 2.5; # Evaluate the sample once before testing the bounds.
    lower := 0.0; # Name the inclusive lower bound without an implicit int conversion.
    upper := 3.0; # Name the exclusive upper bound in the same numeric profile.
    if (sample >= lower && sample < upper || !(sample == sample)) { # && binds above ||; the last relation detects NaN.
        return sample * 17.0; # Publish exactly 42.5 only in the selected arm.
    } else { # This arm is lazy and has its own result-validity flag.
        return -0.0; # Preserve the signed zero fallback as a binary64 value.
    } # Scalar bool merge cells remain private to this execution frame.
} # Dis and SWF execute the same authored branch, with no shared heap.
