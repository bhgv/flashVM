# Syntax-only fixture: name binding, conversion checking and code generation are separate.
implement NamedCasts; # Declare this implementation's name.
Scalar: type int; # A named alias provides a type-shaped conversion prefix.
convert(value: real): Scalar { # The signature uses the alias as its return type.
    return Scalar value; # Conversion uses a named type followed by a unary operand.
} # End the conversion function.
