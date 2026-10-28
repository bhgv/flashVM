# Typed module contract: compile this same file into both Limbo calls and C frame metadata.
Nativedemo: module # A module interface describes exports; it contains no native implementation.
{ # Keep every export inside the generated signature/map contract.
    PATH: con "$Nativedemo"; # Resolve a builtin registered by the matching host, not a .so file.
    add: fn(left: big, right: big): big; # Return the exact low 64 bits of the sum.
    greeting: fn(): string; # Allocate and return an ordinary Dis-owned string.
    check: fn(status: int); # Raise "native demo: rejected" for zero; otherwise return void.
}; # Changing a signature requires regeneration and rebuilding both sides.
