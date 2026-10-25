// Readable AS2 equivalent of the repeated DoAction body in 02-input-audio.swf.
// The actual movie is produced by make_movies.py, including shapes, timeline moves and sound tags.
// This file teaches input actions; it is not an input to our still-limited FIR source compiler.
if (!KeyObserverReady) // Initialize listener/root focus only once across the movie's repeated loops.
{ // Begin persistent setup for the two independent keyboard notification paths.
    KeyObserver = new Object(); // Retain a listener object in the root timeline's variables.
    KeyObserver.downs = 0; // Count delivered presses, including taps between rendered frames.
    KeyObserver.ups = 0; // Count delivered releases separately from the held-key state.


    // Count an ordered Key listener press notification.
    // Parameters: none. Returns no value; this is the registered KeyObserver.
    KeyObserver.onKeyDown = function() // Install a receiver-bound script callback.
    { // Begin one key-press notification.
        this.downs = this.downs + 1; // Update this listener's retained counter.
    }; // Finish the callback without modifying the separate focused-root state.


    // Count an ordered Key listener release notification.
    // Parameters: none. Returns no value; this is the registered KeyObserver.
    KeyObserver.onKeyUp = function() // A release remains observable even after a short tap.
    { // Begin one key-release notification.
        this.ups = this.ups + 1; // Preserve the count across timeline loops.
    }; // End the listener's release callback.

    Key.addListener(KeyObserver); // Root this listener through the VM's supported Key subset.
    FocusDowns = 0; // Keep the focused MovieClip press count independent of KeyObserver.
    FocusUps = 0; // Keep the focused MovieClip release count independent as well.
    _root.focusEnabled = true; // Make the root eligible for the supported MovieClip Selection subset.


    // Count a press delivered to the selected root MovieClip.
    // Parameters: none. Returns no value; this is the focused root, not KeyObserver.
    _root.onKeyDown = function() // Install the independent focus notification path.
    { // Begin the focused-root press callback.
        this.FocusDowns = this.FocusDowns + 1; // Record the event on the selected root object.
    }; // Finish focused press processing.


    // Count a release delivered to the selected root MovieClip.
    // Parameters: none. Returns no value; this is the focused root MovieClip.
    _root.onKeyUp = function() // Focused releases will alter the blue marker height below.
    { // Begin the focused-root release callback.
        this.FocusUps = this.FocusUps + 1; // Preserve releases independently of listener notifications.
    }; // End focused release processing.

    Selection.setFocus("_root"); // Explicitly choose a focus target; mouse/Tab focus is not assumed.
    KeyObserverReady = true; // Prevent duplicate listener registration on the next movie loop.
} // Finish setup; the following input actions execute on every timeline frame.

pointer._x = HostInput.x; // Place the pink marker at the host-mapped stage X coordinate.
pointer._y = HostInput.y; // Use the corresponding stage Y, not raw window/device pixels.
box._x = box._x + (Key.isDown(39) - Key.isDown(37)) * 5; // Right minus Left gives a signed horizontal step.
box._y = box._y + (Key.isDown(40) - Key.isDown(38)) * 5; // Down minus Up gives a signed vertical step.
pointer._xscale = 100 + HostInput.buttons * 10; // Make the current host button bitmask visibly affect width.
box._xscale = 100 + Key.isDown(32) * 80; // Holding Space stretches the blue marker to 180 percent.
box._yscale = 100 + FocusUps * 5; // Each focused-root release increases its height by five percent.
key._x = 20 + Key.getCode() * 4; // Show the most recently pressed virtual-key code as a horizontal position.
key._yscale = 100 + KeyObserver.downs * 5; // Preserve short presses through their listener counter.
key._y = 250 + KeyObserver.ups * 2; // Make accumulated releases visible independently of key holding.
click._x = 20 + HostInput.clicks * 20; // Move the yellow marker for each host-counted primary click.
