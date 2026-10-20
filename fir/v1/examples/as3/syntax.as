// Const bindings are immutable in this adapter, not merely parsed as keywords.
const rate:Number = 2, base:Number = 0x10;
var x = -(base + .5); // Grouped unary minus gives -16.5.
x *= rate; // Reassign a mutable variable with a fresh SSA result.
trace(x); // Expected trace: -33; ABC generation is not implied.
