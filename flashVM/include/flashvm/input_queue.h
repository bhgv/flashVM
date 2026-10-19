/* FlashVM: Public embedding API declarations: input queue.
 * File: include/flashvm/input_queue.h.
 */

#ifndef FLASHVM_INPUT_QUEUE_H
/* Define FLASHVM INPUT QUEUE H constant or compilation switch. */
#define FLASHVM_INPUT_QUEUE_H
#include "input.h"
#include "keyboard.h"
#include "mouse.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* A bounded FIFO for deferred dispatch on the VM thread.
     * This initial queue is NOT thread-safe: producers must call enqueue on
     * the owning thread. Platform threads must marshal through their host loop.
     * Each queued item stores an event copy, not a borrowed pointer.
     */
    /* Copy a validated finite-coordinate event. Invalid input preserves the queue;
     * FVM_INPUT_MAX_PENDING_EVENTS pending items is the fixed capacity. */
    FVM_API FVM_Result fvm_input_enqueue(FVM_Context *, const FVM_InputEvent *, FVM_InputPhase);
    /* Copy a finite world-coordinate primary-pointer transition. No dispatch or pointer
     * publication occurs until pump. Same capacity/owner-thread restrictions as gestures. */
    FVM_API FVM_Result fvm_input_enqueue_mouse(FVM_Context *, FVM_MouseEventType,
                                               double world_x, double world_y);
    /* Copy a validated key snapshot, pressed=0/1, and finite event-time world pointer.
     * Pump publishes that position before invoking the normal rooted keyboard dispatcher. */
    FVM_API FVM_Result fvm_input_enqueue_keyboard(FVM_Context *, const FVM_KeyboardState *,
                                                  int pressed, double world_x, double world_y);
    /* Cancel all pending variants without callbacks or polling-state changes. No allocation.
     * Callable during pump callbacks: stop the old batch even if new work is then enqueued.
     * This does not synthesize up/cancel events or reset held keys; host chooses that policy. */
    FVM_API FVM_Result fvm_input_clear_queue(FVM_Context *);
    /* Deliver only the batch present on entry, up to max_events (0 means the entry batch).
     * Callback-enqueued work waits for another pump. Errors consume the current event and
     * retain later ones; processed counts successful deliveries, may be NULL, is zeroed on
     * invalid input. Recursive pump is rejected; context destruction is blocked during pump.
     * All calls are owner-thread only; teardown/collector entry is invalid. */
    FVM_API FVM_Result fvm_input_pump(FVM_Context *, unsigned int max_events,
                                      unsigned int *processed);
    /* Implement input pending for this subsystem. */
    FVM_API FVM_Result fvm_input_pending(FVM_Context *, unsigned int *count);
#ifdef __cplusplus
}
#endif
#endif
