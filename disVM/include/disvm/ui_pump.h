/* Bounded owner-thread routing between the UI queues and a typed service endpoint. */
#ifndef DISVM_UI_PUMP_H
#define DISVM_UI_PUMP_H              /* Guard the C89 owner-pump ABI. */
#include "ui_bridge.h"
#define DVM_UI_PUMP_BUDGET_MAX 1024U /* Maximum rounds in one explicitly bounded owner step. */

#ifdef __cplusplus
extern "C"
{
#endif


/* Offer borrows message until return. OK means copied/accepted; NOT_FOUND means no transfer.
 * opaque is host-owned endpoint state. Other statuses are terminal; never block or longjmp. */
typedef FVM_Result (*DVM_UIOffer)(void *opaque, const DVM_UIMessage *message);


/* Take writes an owned value record only on OK. NOT_FOUND means no service output ready.
 * opaque is host-owned endpoint state. Never block, reenter the pump or escape by longjmp. */
typedef FVM_Result (*DVM_UITake)(void *opaque, DVM_UIMessage *message);


typedef struct DVM_UIPump DVM_UIPump; /* Single-owner state; bridge and callbacks remain borrowed. */


/* Observable copied routing state; counters saturate rather than wrap. */
typedef struct DVM_UIPumpState
{
    unsigned long to_service; /* Messages actually accepted by the service callback. */
    unsigned long to_ui;      /* Messages actually accepted by the UI queue. */
    int pending_service;      /* A consumed UI record is retained for delivery/recovery. */
    int pending_ui;           /* A consumed service record is retained for delivery/recovery. */
    int stopped;              /* Explicit abort or terminal transport/callback failure. */
    FVM_Result failure;       /* First terminal status; OK until terminal failure. */
} DVM_UIPumpState;


/* Allocate a pump borrowing bridge, offer/take and opaque until destroy.
 * All pointers except opaque must be nonnull; out is unchanged on failure.
 * Returns FVM_OK or validation/allocation failure; performs no callbacks or input. */
FVM_Result dvm_ui_pump_create(DVM_UIBridge *bridge, DVM_UIOffer offer, DVM_UITake take, void *opaque, DVM_UIPump **out);


/* Run up to budget rounds on the service owner, visiting both directions fairly.
 * budget is 1..BUDGET_MAX; no operation waits. Output state is always copied on an admitted step.
 * OK means progress, NOT_FOUND means idle/backpressure. A terminal error retains pending records.
 * Reentry returns ERROR without mutating the active outer step; state must be nonnull. */
FVM_Result dvm_ui_pump_step(DVM_UIPump *pump, unsigned int budget, DVM_UIPumpState *state);


/* Copy current state outside callbacks. pump/state must be valid; owner-thread only.
 * Returns OK or INVALID_ARGUMENT; no queue operation or callback occurs. */
FVM_Result dvm_ui_pump_state(const DVM_UIPump *pump, DVM_UIPumpState *state);


/* Recover one retained value after a failure or while backpressured; does not consume it.
 * toward_service is 0/1, out is required. Returns NOT_FOUND if no such pending record.
 * Cannot be called from a callback; output remains unchanged on failure. */
FVM_Result dvm_ui_pump_pending(const DVM_UIPump *pump, int toward_service, DVM_UIMessage *out);


/* Explicitly stop routing and close both bridge directions; retained records remain inspectable.
 * Returns ERROR on callback reentry, INVALID_ARGUMENT for null, otherwise OK. */
FVM_Result dvm_ui_pump_stop(DVM_UIPump *pump);


/* Free an inactive pump. Caller must first stop/join its owner and inspect pending records.
 * No implicit bridge close, callback or message replay occurs. Returns ERROR if active. */
FVM_Result dvm_ui_pump_destroy(DVM_UIPump *pump);

#ifdef __cplusplus
}
#endif
#endif
