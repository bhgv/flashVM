// Timeline sketch matching executable png-start.swf; make_movies.py emits actual AVM1 bytes.
// Frame 1 creates a stable target and old child before the image request.
_root.createEmptyMovieClip("picture", 1); // Keep the same instance through image/SWF replacement.
_root.picture.createEmptyMovieClip("old", 1); // Its binding must disappear when the image arrives.
_root.picture.kept = 42; // Own data survives every content replacement and unload.
_root.saved = _root.picture; // Compare identity independently of the current content.
_root.iter = 0; // Bound the later two-frame wait loop.
_root.phase = 0; // Import image first, then explicitly unload and reload a SWF.
loadMovie("picture.png", _root.picture); // Global object destination, decoded only by the owner.
// Frame 2 repeats until both content transitions are observed.
++_root.iter; // Count real wait-loop visits without reissuing frame 1.
if (_root.iter > 200 && _root.finished !== 1) // Missing or wrong completion must fail visibly.
{ // This bound is for loopback demonstration, not an Internet timing promise.
    throw "image reload did not finish"; // Surface an ordinary VM failure to the C host.
} // Continue while the image or replacement SWF is pending.
if (_root.phase === 0 && getProperty(_root.picture, 8) === 8) // Index 8 reads native width after import.
{ // The complete image graph has been published at a prior safe point.
    if (_root.picture !== _root.saved || _root.picture.kept !== 42) // Image arrival must retain instance state.
    { // A new object at the same path would violate this contract.
        throw "image target changed"; // Do not hide a lost instance behind a visual success.
    } // Continue only with the original object identity.
    _root.phase = 1; // Prevent the completed image phase from reissuing requests.
    _root.picture.unloadMovie(); // Cancel earlier subtree work and queue empty contents.
    loadMovie("after.swf", _root.picture); // This later reload survives and uses the same destination.
} // All decoding, drawing publication and native cleanup occur outside this callback.
if (_root.picture.loaded === 1) // after.swf sets the marker on its first actual frame.
{ // This proves the replacement SWF was imported and executed, not merely fetched.
    _root.finished = 1; // Mark successful completion for the bounded loop assertion.
    stop(); // The C host still owns window events and eventual joined cleanup.
} // Frame 3 returns to frame 2 if the root remains playing.
// Frame 3: gotoAndPlay(2); // The generator emits the loop directly without an AS compiler.
