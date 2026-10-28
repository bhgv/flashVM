implement IntShifts; # Select the scalar teaching module compiled by Coco/R.
IntShifts: module { # Declare the existing zero-argument entry profile.
    value: fn(): int; # Counts and shifted values are signed32, even on NC64.
}; # The example uses the existing checked FIR shift operations.
value(): int { # Raw checked shifts target Dis; optimized constants can target SWF.
    Count: type int; # Reuse local aliases in annotations and expressions.
    answer: Count = Count 21; # Start from an exact small integer.
    answer <<= 2; # Check count 0..31, then shift left with modulo-2^32 result.
    answer >>= 1; # Check the count again and preserve the sign during right shift.
    return answer; # Report 42 through the teaching result/trace wrapper.
} # Negative counts and counts at least 32 raise instead of being masked.
