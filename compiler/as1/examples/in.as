// Syntax-only property membership and for-in example.
var object = {answer:42};
trace("answer" in object);
for (var key in object) { trace(key); }
for (var present = ("answer" in object); present; present = false) { trace(present); }
