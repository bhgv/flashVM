implement IntBits; # Select the complete scalar source module for both VM targets.
IntBits: module { # The teaching wrapper captures one signed32 result.
    value: fn(): int; # Keep the exported result homogeneous int.
}; # Local aliases will be resolved inside the function body.
value(): int { # All arithmetic below remains exact signed32.
    Number: type int; # An alias changes source spelling without adding runtime storage.
    high: Number = Number -2147483648; # Set only the signed32 sign bit.
    low: Number = Number 2147483647; # Set every remaining bit.
    full := high | low; # Union produces minus one, including correct sign on NC64.
    empty := full ^ ~0; # Complement zero once, then xor identical all-one patterns to zero.
    answer := empty | 42; # Insert the bit pattern for the expected result.
    answer &= 63; # Retain the low six bits using a same-type compound assignment.
    answer ^= 0; # Confirm that exclusive-or preserves the existing pattern.
    return answer; # Both Dis and linked SWF trace exactly 42.
} # The optimizer may fold this entire pure computation to one constant.
