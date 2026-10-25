// Syntax-only Unicode fixture: acceptance does not execute code or resolve bindings.
var \u0061 = 1;       // The escaped spelling denotes the ASCII name a.
var имя = \u0061 + 1; // Cyrillic letters are validated against the pinned category table.
var a\u0301 = имя;    // A combining mark is allowed after a valid initial letter.


function readName()
{
    // Function names use the same identifier and escape rules.
    return a\u0301; // An escaped spelling preserves its original diagnostic byte span.

} // End the function; callers choose whether to type-check or compile this syntax.
