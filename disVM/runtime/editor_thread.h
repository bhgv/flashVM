/* Native Dis/UI ownership split; no VM pointers cross this interface. */
#ifndef DVM_EDITOR_THREAD_H
#define DVM_EDITOR_THREAD_H
#include "disvm/ui_bridge.h"

#define DVM_EDITOR_RPC_BYTES (2UL*1024UL*1024UL) /* Counted geometry bound, independent of GC heaps. */
#define DVM_EDITOR_RPC_START 1 /* Initialize the UI with a copied module directory. */
#define DVM_EDITOR_RPC_ROWS 2 /* Return current row capacity on the UI owner. */
#define DVM_EDITOR_RPC_LINES 3 /* Return startup line-number preference. */
#define DVM_EDITOR_RPC_FRESH 4 /* Return startup fresh-document flag. */
#define DVM_EDITOR_RPC_PATH 5 /* Copy startup path to caller-owned native storage. */
#define DVM_EDITOR_RPC_INK 6 /* Copy only a frozen stroke's encoded samples. */
#define DVM_EDITOR_RPC_ACK 7 /* Bind a frozen token to copied outgoing geometry. */
#define DVM_EDITOR_RPC_CANCEL 8 /* Reject a token on its UI owner. */


/* Run the real Dis entry on one OS worker while the caller owns UI/SDL.
 * entry/user remain borrowed until join. Returns the entry status or1 on a
 * transport/startup failure; all handles are joined before returning. */
int dvm_editor_thread_launch(int (*entry)(void *), void *user);


/* Immutable launch mode; initialized before worker creation and reset after join.
 * Only the launcher creates/destroys shared state. */
int dvm_editor_threaded(void);


/* Synchronous Dis-owner RPC. No Dis heap pointer is accepted: input bytes are
 * copied before waiting; output is malloc-owned and must be freed by caller.
 * UI never waits on this API. Returns1 on delivery,0 on failure/shutdown. */
int dvm_editor_thread_rpc(unsigned int operation, unsigned long token,
    const char *input, size_t size, long *scalar, char **output);


/* Latch owner shutdown before closing queues. Safe to call repeatedly. */
void dvm_editor_thread_stopping(void);


/* Thread-safe failure observation for the Dis scheduler between turns. */
int dvm_editor_thread_failed(void);


/* Observe shutdown with the same mutex used before transport closure. */
int dvm_editor_thread_is_stopping(void);


/* Main-owner delivery remains direct in sequential mode; threaded mode sends a
 * copied record to the UI queue, blocking only the Dis worker under backpressure. */
int dvm_editor_deliver(const DVM_UIMessage *message);


/* Queue access/lifecycle split implemented by editor_host.c. Setup/finish/UI
 * poll run only on main; service poll/stop run only on the Dis owner. */
int dvm_editor_transport_prepare(void);
void dvm_editor_transport_finish(void);
int dvm_editor_ui_poll(int stopping);
DVM_UIBridge *dvm_editor_transport(void);
#endif
