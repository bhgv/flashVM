// ActionScript 2.0 conceptual comparison (not a FlashVM integration test).
// ActionScript strings use UTF-16-like character semantics; the native
// UTF-8 helper functions are NOT yet exposed to ActionScript automatically.
var greeting:String = "Hello, мир!";
trace(greeting.length);
trace(greeting.indexOf("Hello"));
