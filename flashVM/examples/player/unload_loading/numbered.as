// Explanatory timeline for executable numbered_start.swf; the standalone generator emits its bytes.
_root.createEmptyMovieClip("child", 1); // Queue a child request which a following root unload cancels.
_root.child.loadVariables("never.txt"); // This obsolete form must never reach the server.
loadMovieNum("never.swf", 0); // An earlier root request must also be removed without I/O.
unloadMovieNum(0); // Clear the root through the common deferred lifecycle and cancellation queue.
loadMovieNum("numbered_next.swf", "0", "GET"); // Numeric string zero is accepted; this later request survives.
// The old second frame throws if replacement failed; numbered_next.swf owns its own source provenance.
// The replacement requests fresh.txt, waits on two frames, checks answer==fresh, then stops.
// Levels other than zero and POST currently return UNSUPPORTED; no additional display level is created.
