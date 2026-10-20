// Human-readable equivalent of the generated movie; broad syntax, not FIR numeric lowering.
var observer = new Object(); // Keep one observer alive through Mouse.addListener.
observer.onMouseWheel = function(delta) { // Receive host-normalized vertical scroll lines.
    trace(delta); // Print positive upward or negative downward movement to the player's console.
}; // Complete the wheel callback without requesting timeline advancement.
observer.onMouseDown = function() { // Receive the primary button/contact press.
    Mouse.hide(); // Request native cursor hiding at the next player safe point.
}; // End the press callback; no SDL calls occur inside the VM.
observer.onMouseUp = function() { // Receive the primary button/contact release.
    Mouse.show(); // Request native cursor restoration without another timeline frame.
}; // End the release callback.
Mouse.addListener(observer); // Register once; the context roots the listener until teardown.
stop(); // Stop this one-frame timeline while the player's input loop remains active.
