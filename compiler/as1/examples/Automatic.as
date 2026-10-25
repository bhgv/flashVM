// Syntax example: automatic semicolons and classic Flash authoring wrappers.
var total = 0 // A line break ends this variable declaration.


// Add one untyped input to the shared total.
// Parameters: value is the amount added. Returns the updated total.
function accumulate(value) { // Untyped AS1/AS2 function accepted by both syntax profiles.
    total += value // A line break separates assignment from return.
    return total // A closing brace also permits an omitted semicolon.
} // Finish the function declaration; no runtime is invoked by the parser.
on(release, keyPress "<Space>") { // Contextual event list, not an ordinary function call.
    accumulate(1) // Parse a call inside the authoring wrapper.
} // Event execution requires a later authoring/lowering implementation.
