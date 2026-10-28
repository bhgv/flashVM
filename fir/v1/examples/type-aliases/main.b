implement Demo; # Select the implementing module; aliases below belong to this file.
Count: type Base; # Save a forward edge in file scope; module shadows cannot rebind it.
Base: type int; # Complete the file-scope chain before prefix validation.
Demo: module { # Open the implementing module and its inner type scope.
    Base: type real; # Shadow the outer name without changing Count.
    value: fn(): Count; # Export the teaching-profile entry with canonical int result.
}; # Close the interface before the executable body.
value(): Count { # The body sees both lexical alias scopes.
    answer: Count = Count 42; # An identity cast through the alias emits no extra FIR operation.
    return Count answer; # The teaching wrapper traces 42 and stores the result in Dis data.
} # End the only supported entry function in this example.
