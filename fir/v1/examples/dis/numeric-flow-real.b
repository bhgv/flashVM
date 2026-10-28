implement NumericFlowReal; # Select the module for the Coco/R real source profile.
NumericFlowReal: module { # Keep the public teaching interface explicit and homogeneous.
    value: fn(): real; # No external function-call or module ABI is implied.
}; # Compilation still produces the existing main/trace execution harness.
value(): real { # Every reachable exit must return a binary64 value.
    Measure: type real; # Resolve this local alias without runtime storage.
    sample: Measure = Measure 1.0; # Initialize a numeric local before any control-flow edge.
    while (sample < 32.0) { # Re-evaluate the current value through a private f64 cell each pass.
        sample *= 2.0; # Store the updated binary64 value before the backedge.
    } # The zero-iteration path and every backedge share the same dominating cell.
    if (sample == 32.0) { # This decision may rejoin instead of returning in each arm.
        sample += 10.5; # Update only the selected path to exactly 42.5.
    } else { # The optional false arm uses the same declared numeric type.
        sample = -0.0; # A real cell preserves this fallback's signed-zero payload.
    } # Both open paths now join before the subsequent numeric read.
    return sample; # Capture exactly 42.5 and one validity flag in interpreter/JIT and SWF.
} # Cells are scalar frame storage, never GC roots or exported native addresses.
