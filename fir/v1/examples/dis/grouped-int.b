implement GroupedInt; # Select a separate module name for this independent object.
GroupedInt: module { # No module loader or native pointer is needed by this example.
    value: fn(): int; # Every numeric binding and result remains signed32.
}; # Comparisons produce internal normalized bools only inside expressions.
value(): int { # The compiler preserves exact wraparound arithmetic semantics.
    mask := 3; # Store the bit mask as an int, not a condition.
    bits := 1 | 2; # Arithmetic grouping below makes the bitwise precedence explicit.
    if ((((bits & mask) == 3) || (0 == 1)) && !((bits < 0) != (bits > mask))) { # Group lazy branches and compare the two normalized predicates.
        return ((bits + 4) * 6); # Return exactly 42 through both numeric backends.
    } else { # This arm remains lazy and publishes a separate capture flag.
        return -7; # Keep the result homogeneous even on the fallback path.
    } # No bool local, implicit numeric truthiness or cross-heap owner is introduced.
} # All nonblank source lines explain the language and execution contracts.
