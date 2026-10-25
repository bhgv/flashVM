/* Owner-thread FIFO for captured FlashVM input and UI service requests. */
#ifndef DISVM_UI_CLIENT_H
#define DISVM_UI_CLIENT_H          /* Guard the C89 client-queue declarations. */
#include "ui_bridge.h"
#define DVM_UI_CLIENT_CAPACITY 64U /* Locally accepted records, additional to the bounded bridge queue. */

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct DVM_UIClient DVM_UIClient; /* Borrows a bridge; never owns a FlashVM context. */


/* Allocate a UI-owner client borrowing bridge until destroy; out changes only on success.
 * Returns OK or validation/allocation error; no callbacks or channel operations occur. */
FVM_Result dvm_ui_client_create(DVM_UIBridge *bridge, DVM_UIClient **out);


/* Validate and copy message into the bounded local FIFO, without sending immediately.
 * client/message are borrowed. NOT_FOUND means caller retains the unaccepted record.
 * OK means local acceptance, not service execution. All producers must use this FIFO
 * when their relative ordering matters; bypass sends can overtake local pending input. */
FVM_Result dvm_ui_client_post(DVM_UIClient *client, const DVM_UIMessage *message);


/* Flush at most budget records without blocking; budget is 1..CLIENT_CAPACITY.
 * sent receives actual transport acceptances, including before an eventual error.
 * Returns OK for progress, NOT_FOUND for idle/full, or a sticky terminal channel error.
 * Every unaccepted record remains owned by the client for inspection/recovery. */
FVM_Result dvm_ui_client_flush(DVM_UIClient *client, unsigned int budget, size_t *sent);


/* Inspect the oldest locally retained record without consuming it.
 * client/out/pending are required. pending receives the local count on success or empty.
 * Returns NOT_FOUND when empty; out is unchanged then. */
FVM_Result dvm_ui_client_peek(const DVM_UIClient *client, DVM_UIMessage *out, size_t *pending);


/* Free a quiescent client without closing the borrowed bridge or replaying pending records.
 * Caller must inspect/recover pending values before teardown. Null is allowed; returns nothing. */
void dvm_ui_client_destroy(DVM_UIClient *client);

#ifdef __cplusplus
}
#endif
#endif
