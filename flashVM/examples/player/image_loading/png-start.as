// Timeline sketch matching executable png-start.swf; make_movies.py emits actual AVM1 bytes.
// Frame 1 creates a stable target and old child before the image request.
_root.createEmptyMovieClip("picture", 1);     // Keep the same instance through image/SWF replacement.
_root.picture.createEmptyMovieClip("old", 1); // Its binding must disappear when the image arrives.
_root.picture.kept = 42;                      // Own data survives every content replacement and unload.
_root.saved = _root.picture;                  // Compare identity independently of the current content.
_root.iter = 0;                               // Bound the later two-frame wait loop.
_root.phase = 0;                              // Import image first, then explicitly unload and reload a SWF.
loadMovie("picture.png", _root.picture);      // Global object destination, decoded only by the owner.
// Frame 2 repeats until both content transitions are observed.
++_root.iter;                                 // Count real wait-loop visits without reissuing frame 1.
if (_root.iter > 200 && _root.finished !== 1) // Missing or wrong completion must fail visibly.
{ // Begin the documented branch/scope: Missing or wrong completion must fail visibly.
    // This bound is for loopback demonstration, not an Internet timing promise.
    throw "image reload did not finish"; // Surface an ordinary VM failure to the C host.
} // End the documented branch/scope.

// Continue while the image or replacement SWF is pending.
if (_root.phase === 0 && _root.picture._width === 8) // Named width reads the same native rectangle as index 8.
{ // Begin the documented branch/scope: Named width reads the same native rectangle as index 8.
    // The complete image graph has been published at a prior safe point.
    if (_root.picture !== _root.saved || _root.picture.kept !== 42) // Image arrival must retain instance state.
    { // Begin the documented branch/scope: Image arrival must retain instance state.
        // A new object at the same path would violate this contract.
        throw "image target changed"; // Do not hide a lost instance behind a visual success.
    } // End the documented branch/scope.

    // Continue only with the original object identity.
    if (_root.picture.getBytesLoaded() !== _root.picture.getBytesTotal() || // Complete encoded input has arrived.
        _root.picture.getSWFVersion() !== -1)                               // Images have no own SWF header version.
    { // Begin the documented branch/scope: Images have no own SWF header version.
        // Imported images have encoded byte counts and no own SWF header version.
        throw "content query mismatch"; // Propagate a real metadata failure to the host.
    } // End the documented branch/scope.

    // The executable generator additionally checks the exact encoded payload length.
    _root.picture._width = 16;  // Resize through the named width setter after decoding.
    _root.picture._height = 24; // Both setters reuse the numeric AVM1 scaling rules.
    if (getProperty(_root.picture, 8) !== 16 || getProperty(_root.picture, 9) !== 24) // Compare numeric geometry.
    { // Begin the documented branch/scope: Compare numeric geometry.
        // Named setters and legacy instructions must describe the same shape.
        throw "image dimension mismatch"; // Report a real VM error to the host.
    } // End the documented branch/scope.

    // Proceed with the resized, still identical target.
    _root.imageOrigin = _root.picture._url;       // Save the loader's managed source location.
    _root.picture._url = "ignored source";        // Readonly assignment must preserve the private base.
    if (_root.picture._url !== _root.imageOrigin) // Do not silently corrupt later relative loading.
    { // Begin the documented branch/scope: Do not silently corrupt later relative loading.
        // URL mutation would break source ownership and relative reloads.
        throw "readonly image origin changed"; // Make provenance regressions visible.
    } // End the documented branch/scope.

    // The next SWF request still uses the original source directory.
    _root.phase = 1;                       // Prevent the completed image phase from reissuing requests.
    _root.picture.unloadMovie();           // Cancel earlier subtree work and queue empty contents.
    loadMovie("after.swf", _root.picture); // This later reload survives and uses the same destination.
} // End the documented branch/scope.

// All decoding, drawing publication and native cleanup occur outside this callback.
if (_root.picture.loaded === 1) // after.swf sets the marker on its first actual frame.
{ // Begin the documented branch/scope: after.swf sets the marker on its first actual frame.
    // This proves the replacement SWF was imported and executed, not merely fetched.
    if (_root.picture.getSWFVersion() !== 8 ||                            // Verify the replacement SWF header version.
        _root.picture.getBytesLoaded() !== _root.picture.getBytesTotal()) // Canonical bytes are fully published.
    { // Begin the documented branch/scope: Canonical bytes are fully published.
        // Canonical SWF bytes are fully available after owner publication.
        throw "replacement content mismatch"; // Keep success tied to actual imported metadata.
    } // End the documented branch/scope.

    // The executable generator also checks the exact length of after.swf.
    _root.finished = 1; // Mark successful completion for the bounded loop assertion.
    stop();             // The C host still owns window events and eventual joined cleanup.

} // Frame 3 returns to frame 2 if the root remains playing.
// Frame 3: gotoAndPlay(2); // The generator emits the loop directly without an AS compiler.
