// Root timeline sketch for root_start.swf; the independent generator emits the executable bytes.
_root.createEmptyMovieClip("child", 1); // Retain a child request which the later root clear cancels.
_root.kept = 42;                        // This own property must survive content clearing.
_root.turn = 0;                         // Count retained callback deliveries independently of the old frame list.
_root.child.loadVariables("slow.txt");  // The supplied loopback server delays this obsolete response.
_root.loadVariables("never.txt");       // This earlier root request must be dropped before I/O.
_root.onEnterFrame = function()         // The root instance and handler survive the content replacement.
{ // Begin the documented branch/scope: The root instance and handler survive the content replacement.
    // Enter on the VM owner thread, never a native network worker.
    ++_root.turn;         // Count actual callback executions after installation.
    if (_root.turn === 1) // Clear only once rather than repeatedly replacing new content.
    { // Begin the documented branch/scope: Clear only once rather than repeatedly replacing new content.
        // The native callback queues the operation until the next owner safe point.
        unloadMovie("_level0"); // Clear the root, cancel earlier subtree loads and release native root audio.
        loadVariables("fresh.txt", _root); // This later request survives and keeps the original network base.
    } // End the documented branch/scope.

    // The current script returns before native cleanup can join another thread.
    if (_root.turn > 1) // Only subsequent turns may inspect the committed empty root.
    { // Begin the documented branch/scope: Only subsequent turns may inspect the committed empty root.
        // Root content now contains one stopped frame without the old child name.
        if (_root.kept !== 42 || _root._totalframes !== 1 || _root.child !== undefined) // Validate retained state.
        { // Begin the documented branch/scope: Validate retained state.
            // Treat lifecycle errors as visible playback failures.
            throw "root state changed"; // Propagate ordinary VM error to the C host.
        } // End the documented branch/scope.

        // Continue with the original root instance and callback.
    } // End the documented branch/scope.

    // Stopped timelines still receive retained onEnterFrame callbacks.
    if (_root.answer === "fresh") // Only a post-unload relative form response proves successful reload.
    { // Begin the documented branch/scope: Only a post-unload relative form response proves successful reload.
        // Native resource bytes have been assigned on the owner thread.
        _root.finished = 1; // Mark success independently of the removed old frame actions.
    } // End the documented branch/scope.

    // The host remains responsive even though the root has no drawing.
    if (_root.turn > 100 && _root.finished !== 1) // Bound this loopback demonstration's completion wait.
    { // Begin the documented branch/scope: Bound this loopback demonstration's completion wait.
        // This is a fixture bound, not an Internet timing guarantee.
        throw "relative reload did not arrive"; // Fail rather than silently reaching the host frame limit.
    } // End the documented branch/scope.

    // No synthetic unload event is required for this callback-driven demonstration.

}; // The generator also adds looping PCM and a forbidden old third-frame action.
