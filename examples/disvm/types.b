implement Types; # Define the module loaded by DisVM.
include "sys.m"; # Import the hosted console interface.

Types: module { # Declare the hosted entry point.
    init: fn(); # DisVM calls init with no arguments.
}; # End of the module interface.

Point: adt { # Define a named aggregate.
    x: int; # Store a signed coordinate.
    y: int; # Store another signed coordinate.
}; # End of the aggregate declaration.

init() # Run the data model checks.
{ # Open the entry function.
    sys := load Sys Sys->PATH; # Resolve the local Sys module.
    octet := byte 255; # Convert an integer to an unsigned byte.
    count := -42; # Infer int from the signed literal.
    wide := big 1 << 40; # Use big for a value outside the Int32 range.
    fraction := 1.25; # Infer real from a floating-point literal.
    greeting := "Привет"; # Store Unicode text.
    utf := array of byte greeting; # Encode the text as UTF-8 bytes.
    point := Point(3, 4); # Construct an ADT value.
    selected := ref point; # Allocate a reference to an ADT value.
    points := array[] of {point, Point(5, 6)}; # Store values in a mutable array.
    names := "первый" :: "второй" :: nil; # Build a list from two cons cells.
    pair := (count, greeting); # Construct a tuple with mixed element types.
    (number, text) := pair; # Destructure the tuple into two variables.
    if (int octet != 255 || number != -42 || wide >> 40 != big 1) # Check integer representations.
        raise "integer values"; # Report a failed integer check.
    if (fraction * 4.0 != 5.0 || len utf != 12 || text != greeting) # Check REAL and UTF-8.
        raise "real or text values"; # Report a failed numeric or text check.
    if (selected.y != 4 || points[1].x != 5 || len names != 2 || hd names != "первый") # Check aggregates.
        raise "aggregate values"; # Report a failed aggregate check.
    sys->print("PASS DisVM types\n"); # Print the expected success marker.
} # Close the entry function.
