// Numeric lowering example: one flat scope, initialized bindings, no hoisting.
var x: Number = 6; // First binding; the current value is constant six.
var y = x * 7;     // A second binding receives a new SSA result: forty-two.
x = y - x;         // Reassignment selects a fresh SSA value; y is unchanged.
trace(x);          // Expected first trace: 36.
trace(y);          // Expected second trace: 42.
