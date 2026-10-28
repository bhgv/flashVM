implement Demo; # Select the implementing module.
Result: type Wide; # Keep this forward target in file scope.
Wide: type big; # Complete the result alias with signed 64-bit arithmetic.
Demo: module { # Start the implementing-module type environment.
    I: type Narrow; # Forward module alias used as a cast operator below.
    Narrow: type int; # int conversion narrows the parsed signed64 value modulo 2^32.
    Text: type string; # Use the same canonical string conversion as the primitive spelling.
    value: fn(): Result; # The teaching wrapper exposes one big result.
}; # All aliases, including unused declarations, are validated at this boundary.
value(): Result { # Enter the existing typed big/table expression profile.
    digits: Text = Text "4294967295"; # Keep the exact decimal text in a rooted string value.
    narrowed: I = I digits; # Parse signed64 first; the signed32 result is minus one.
    return Result narrowed + Wide 43; # Widen minus one, add 43 and report exactly 42.
} # The wrapper retires retained values after trace and return.
