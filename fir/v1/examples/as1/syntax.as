// Three initialized numeric bindings; declarations are evaluated left-to-right.
var a = 0x10, b = .5, c = 2.;
a += 2; // a becomes eighteen; no old SSA definition is overwritten.
b *= 4; // b becomes two; multiplication uses the AS f64 numeric profile.
c -= 1; // c becomes one.
trace(-(a + b) * +c); // Parenthesized unary minus and unary plus produce -20.
