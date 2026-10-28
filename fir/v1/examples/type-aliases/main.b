implement Demo; # Select the implementing module; aliases below belong to this file.
Base: type int; # Bind the initial scalar alias before the module scope exists.
Count: type Base; # Capture int now; later shadowing cannot rebind this alias.
Demo: module { # Open the implementing module and its inner type scope.
    Base: type real; # Shadow the outer name without changing Count.
    value: fn(): Count; # Export the teaching-profile entry with canonical int result.
}; # Close the interface before the executable body.
value(): Count { # The body sees both lexical alias scopes.
    answer: Count = 42; # Emit the same int binding as a direct int annotation.
    return answer; # The teaching wrapper traces 42 and stores the result in Dis data.
} # End the only supported entry function in this example.
