/* Native FIFO presentation ordering; capture continuation does not need a new hit. */
#ifndef DVM_EDITOR_POINTER_GATE_H
#define DVM_EDITOR_POINTER_GATE_H

/* Copy boolean owner/FIFO state. Return1 when matching pixels are required.
 * Replay/capture never bypass the barrier. Native keys and an already captured
 * header continuation can proceed; new pointer/focus/lifecycle events cannot. */
static int dvm_editor_pointer_waits(int dirty, int pending, int replay, int capture, int keyboard, int continuation)
{
    return (dirty || pending) && (replay || capture || (!keyboard && !continuation));
}
#endif
