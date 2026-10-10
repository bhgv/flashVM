// ActionScript 2 (SWF 7/8): conceptual target of translated mouse events.
// The input mapper currently returns a button to the host; it does NOT yet dispatch it to AVM1.
var listener:Object = new Object();
listener.onMouseDown = function() { trace("mouse down"); };
Mouse.addListener(listener);
