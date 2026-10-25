# One Dis-side implementation of the interface also used for external AS/C modules.
implement Arithmetic;                     # Implement the module identity declared in arithmetic.m.
include "arithmetic.m";                   # Type-check both callable entries against the shared header.


# Add copied signed 32-bit arguments using the language's int arithmetic.
# Parameters: a and b are input values. Returns their wrapped signed i32 sum.
add(a, b: int): int                        # The native bootstrap can compile this definition today.
{                                         # Begin this call's local execution scope.
    return a + b;                         # Return a scalar value; no pointer or mutable object is exported.
}                                         # End add; no per-call native resource requires cleanup.


# Construct a greeting without sharing storage with another engine.
# Parameters: name is a Limbo string value. Returns a new concatenated string.
# A future checked bridge must copy/serialize that value into the SWF owner.
greet(name: string): string               # The declared result matches the shared .m contract.
{                                         # Begin the implementation's local scope.
    return "Hello, " + name;              # Concatenation creates a managed string, retained by the return value.
}                                         # End greet; the Dis heap retains the returned string as required.
