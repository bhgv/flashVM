// Timeline sketch for make_movies.py; the generated SWF is executable without an AS compiler.
// Frame 1 creates stable child identities and queues three requests.
_root.createEmptyMovieClip("child", 1); // Own the instance which will retain its placement and properties.
_root.child.createEmptyMovieClip("grand", 1); // Create a descendant whose queued request becomes obsolete.
_root.createEmptyMovieClip("peer", 2); // Keep an unrelated target to demonstrate preserved FIFO order.
_root.child.kept = 42; // An own property survives clearing the clip's contents.
_root.saved = _root.child; // Retain identity independently of the script path.
_root.iter = 0; // Bound the later wait loop.
_root.child.loadVariables("slow.txt"); // Start an intentionally delayed request containing poison=1.
_root.child.grand.loadVariables("never.txt"); // This queued descendant request must never perform I/O.
_root.peer.loadVariables("peer.txt"); // Unrelated pending work must survive the unload.
// Frame 2 clears contents and requests fresh data on the retained instance.
unloadMovie("child"); // Queue cleanup; the owner cancels earlier subtree loads at its next safe point.
_root.child.loadVariables("fresh.txt"); // This later request survives and uses the retained source base.
// Frame 3 loops until the later request arrives; frame 4 returns to frame 3.
++_root.iter; // Count only completion checks, never repeat initialization.
if (_root.iter > 250 && _root.finished !== 1) // Detect failure instead of silently reaching the host frame limit.
{ // Keep this a bounded loopback example rather than an Internet latency promise.
    throw "unload reload did not complete"; // Surface ordinary VM failure to the C host.
} // Continue while the later reply is pending.
if (_root.child.answer === "fresh") // Wait for post-unload data, not the cancelled old response.
{ // Validate identity, retained state, deletion and unrelated delivery.
    if (_root.saved !== _root.child || _root.child.kept !== 42) // The instance must remain stable.
    { // A replacement object would lose host/script identity.
        throw "instance state changed"; // Report the mismatch through the player result.
    } // The instance remains attached at its old placement.
    if (_root.child.grand !== undefined || _root.child.poison !== undefined) // No stale contents may return.
    { // Child name bindings and cancelled form fields must remain absent.
        throw "obsolete content survived"; // Fail visibly rather than printing a success marker.
    } // Continue only when cancellation is observable in script state.
    if (_root.peer.answer !== "peer") // The unrelated earlier queued request must complete.
    { // Preserve FIFO ordering outside the cleared subtree.
        throw "unrelated request lost"; // Expose accidental whole-queue cancellation.
    } // All required resource effects now agree.
    _root.child.unloadMovie(); // Repeat through the method form without deleting the instance or its fields.
    _root.finished = 1; // Mark successful bounded completion.
    stop(); // The host continues native ownership and eventually joins/releases everything.
} // Frame 4 executes gotoAndPlay(3) only while this root is still playing.
// Frame 4: gotoAndPlay(3); // The generator emits the two-frame wait loop directly.
