# Typed module contract: compile this same file into both Limbo calls and C frame metadata.
Nativedemo: module # A module interface describes exports; it contains no native implementation.
{ # Keep every export inside the generated signature/map contract.
    PATH: con "$Nativedemo"; # Resolve a builtin registered by the matching host, not a .so file.
    Value: type big; # Semantic alias: the C ABI remains LONG, independent of this source spelling.
    Text: type string; # Semantic alias: the generated GC map still roots String pointers.
    add: fn(left: Value, right: Value): Value; # Return the exact low 64 bits of the sum.
    greeting: fn(): Text; # Allocate and return an ordinary Dis-owned string.
    check: fn(status: int); # Raise "native demo: rejected" for zero; otherwise return void.
}; # Changing a signature requires regeneration and rebuilding both sides.
