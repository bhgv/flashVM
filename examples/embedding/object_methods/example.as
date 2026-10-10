// ActionScript 1/2, conceptual example of the intended CallMethod bridge.
// The host creates receiver and registers the native function "add".
// This call syntax is not yet a verified SWF integration scenario:
var answer = receiver.add(7);
trace(answer); // Expected: 42 when method binding is implemented.
