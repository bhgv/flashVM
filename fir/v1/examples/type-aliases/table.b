implement Demo; # Select this complete source module.
Demo: module { # Open the module-local alias scope.
    Number: type big; # Use signed 64-bit values in the native table profile.
    Values: type table of big; # Alias the FlashVM hybrid table extension.
    Text: type string; # Alias string annotations for table keys.
    value: fn(): Number; # Declare the numeric result consumed by the teaching wrapper.
}; # Close the interface.
value(): Number { # Enter the supported big/table source profile.
    values: Values = table of big {type = big 1}; # Construct a table with a named string field.
    key: Text = "type"; # Store a string key whose spelling is also a parser keyword.
    values.type = big 42; # Dot syntax is the same string-key operation as ["type"].
    return values[key]; # Read 42 through the original string key.
} # End the example; the wrapper releases its table roots after execution.
