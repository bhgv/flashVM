implement Owners; # Select the source module compiled by our Coco/R frontend.
Result: type Wide; # Resolve this forward result alias in file scope.
Wide: type big; # Keep arithmetic exact in signed 64-bit values.
Owners: module { # Declare aliases consumed by the executable typed profile.
    Text: type string; # String casts allocate/retain native String owners.
    Narrow: type int; # Checked decimal parsing precedes signed32 narrowing.
    Values: type table of big; # Hybrid table handles belong to the Dis host.
    value: fn(): Result; # Export the existing result-capture teaching entry.
}; # All type edges are validated before the body is lowered.
value(): Result { # Begin one execution with fresh table/string/key owners.
    values: Values = table of big {["42"] = big 7}; # Create one string-keyed entry.
    snapshot := keysof values; # Retain an independent key snapshot across mutations.
    delete values["42"]; # Remove the live entry without invalidating the saved key.
    sum := big 0; # Keep an observable accumulator in a rooted loop frame.
    for (i := big 0; i < 64; i++) { # Cross native backedges and GC scheduling boundaries.
        digits: Text = Text (i + big 4294967295); # Format a distinct value on every pass.
        narrowed: Narrow = Narrow digits; # Round-trip through text and signed32 narrowing.
        sum += Result narrowed; # Accumulate minus one through sixty-two: 1952 total.
        values[digits] = Result narrowed; # Copy each string key into a separate table owner.
        delete values[digits]; # Release that copied key while the local string remains live.
    } # Retire body locals while preserving outer cells and the independent snapshot.
    if (sum == 1952 && countof values == 0) { # Verify all iterations and absence of both dense and dictionary entries.
        return Result (Narrow snapshot[0]); # Parse the retained deleted key and return 42.
    } else { # Make an incorrect loop or owner result observable to the test harness.
        return Result 0; # Report failure through a separately captured terminal arm.
    } # Both arms terminate through the existing Dis result wrapper.
} # Native host handles and string roots must be released after the completed run.
