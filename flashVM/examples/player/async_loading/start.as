// Timeline sketch matching make_movies.py; generated SWF bytes do not depend on an unfinished AS backend.
// Frame 1 runs once and creates the resource target before issuing requests.
_root.iter = 0; // Count later wait-frame visits so failed completion is observable.
_root.createEmptyMovieClip("child", 1); // Keep one stable identity while its movie bytes arrive.
loadMovie("nested/child.swf", "child"); // Queue a child request without stopping the parent timeline.
_root.loadVariables("root-vars.txt"); // Queue root fields behind the child using the same FIFO.
function pulse() // A real script timer must continue during asynchronous resource work.
{ // Begin the callback body, executed only on the VM owner thread.
    _root.timer_ran = 1; // Mark timer delivery independently of network timing.
} // End the callback; no VM pointer is passed to a native worker.
_root.timer = setInterval(pulse, 10); // Save the timer ID so completion can remove its root.

// Frame 2 remains reachable whether replies are fast or delayed.
_root.frame_ran = 1; // Demonstrate timeline advancement while resources may still be pending.

// Frame 3 waits for both independent origins and an actual timer callback.
++_root.iter; // Bound this tutorial's wait-loop checks.
if (_root.iter > 200 && _root.finished !== 1) // Wrong or missing form values must fail visibly.
{ // This is a loopback tutorial bound, not a general Internet timeout policy.
    throw "asynchronous resources did not complete"; // Surface an assertion failure to the C host.
} // The native host still joins every worker during error cleanup.
if (_root.greeting === "hello from root" // Root form arrived at its own target.
    && _root.child.answer === "child origin works" // Child-relative form arrived.
    && _root.timer_ran === 1) // A real script timer has run before completion.
{ // Apply no native transport operations from this callback.
    if (_root.frame_ran !== 1) // Require evidence that the parent frame actually ran.
    { // A missing frame would invalidate the demonstration.
        throw "timeline did not run"; // Preserve the failure through the ordinary VM status.
    } // Continue only with a real frame marker.
    clearInterval(_root.timer); // Release the explicit repeating callback.
    _root.finished = 1; // Mark this completed tutorial state.
    stop(); // Stop this timeline while the host can still process window events.
} // If not ready, let frame 4 return here without reissuing frame 1.

// Frame 4 forms a two-frame wait loop, never revisiting request initialization.
gotoAndPlay(3); // Continue checking completion on the owner thread.
