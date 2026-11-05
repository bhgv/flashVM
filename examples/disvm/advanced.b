implement Advanced; # Define the module for advanced Limbo types.
include "sys.m"; # Import console output.

Advanced: module { # Declare the hosted module interface.
    init: fn(); # Expose the zero-argument entry point.
}; # End the interface.

Eighths: type fixed(0.125); # Declare a fixed-point scale.
Rejected: exception(int); # Declare a typed exception payload.

Box: adt[T] { # Declare a parameterized aggregate.
    item: T; # Store the instantiated element type.
}; # End the generic declaration.

Choice: adt { # Declare a tagged aggregate.
    pick { # Select fields using a discriminant.
    Number => # Select the numeric variant.
        value: int; # Store its numeric payload.
    Label => # Select the text variant.
        text: string; # Store its text payload.
    } # End the variants.
}; # End the aggregate.

increment(value: int): int # Declare a function with a result.
{ # Open the function.
    return value + 1; # Return the incremented value.
} # Close the function.

init() # Exercise the type declarations.
{ # Open the entry function.
    sys := load Sys Sys->PATH; # Resolve the hosted console.
    callback: ref fn(value: int): int = increment; # Store a typed function reference.
    amount := Eighths(1.25); # Represent ten eighths.
    extra := Eighths(0.5); # Represent four eighths.
    box := Box[string]("payload"); # Instantiate the generic aggregate for string.
    choice: ref Choice = ref Choice.Number(callback(6)); # Allocate the numeric variant.
    received := 0; # Reserve a variable for the exception payload.
    { # Open the protected body.
        raise Rejected(callback(6)); # Raise a typed exception with a value.
    } exception error { # Bind the exception for pattern matching.
        Rejected => (received) = error; # Extract the integer payload.
        * => raise; # Propagate any other exception.
    } # Close the exception handler.
    pick selected := choice { # Inspect the tagged value safely.
        Number => # Handle the numeric variant.
            if (selected.value != 7) # Verify the payload.
                raise "pick"; # Report an incorrect value.
        Label => raise "unexpected label"; # Reject the other variant here.
    } # Close the pick statement.
    if (received != 7 || real (amount + extra) != 1.75 || box.item != "payload") # Check all results.
        raise "advanced types"; # Report a failed type check.
    sys->print("PASS DisVM advanced\n"); # Print the expected marker.
} # Close the entry function.
