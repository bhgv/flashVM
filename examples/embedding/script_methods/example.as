// ActionScript 1/2 illustration of the supported subset.
// The host creates _global.receiver and sets receiver.base = 35.
function compute(n) {
    return this.base + n;
}
var result = receiver.compute(7); // 42
// Note: FlashVM currently resolves the named function from its global
// script-function registry; it does NOT yet store a function value on receiver.
