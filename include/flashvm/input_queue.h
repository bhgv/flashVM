#ifndef FLASHVM_INPUT_QUEUE_H
#define FLASHVM_INPUT_QUEUE_H
#include "input.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A bounded FIFO for deferred dispatch on the VM thread.
 * This initial queue is NOT thread-safe: producers must call enqueue on
 * the owning thread. Platform threads must marshal through their host loop.
 * Each queued item stores an event copy, not a borrowed pointer.
 */
FVM_API FVM_Result fvm_input_enqueue(FVM_Context *, const FVM_InputEvent *, FVM_InputPhase);
/* Dispatch up to max_events (0 means all). On callback failure the item is
 * already removed; callbacks must not destroy the context or reenter pump.
 * processed may be NULL. */
FVM_API FVM_Result fvm_input_pump(FVM_Context *, unsigned int max_events, unsigned int *processed);
FVM_API FVM_Result fvm_input_pending(FVM_Context *, unsigned int *count);
#ifdef __cplusplus
}
#endif
#endif
