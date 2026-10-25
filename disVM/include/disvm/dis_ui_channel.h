/* Native Dis channel boundary; requires the interpreter scheduler/heap owner. */
#ifndef DISVM_DIS_UI_CHANNEL_H
#define DISVM_DIS_UI_CHANNEL_H /* Guard the explicit Dis heap adapter ABI. */
#include "ui_bridge.h"

struct Channel; /* Inferno typed channel, borrowed and rooted by the embedding host. */
struct Array;   /* Inferno byte array; never exported as a pointer into FlashVM. */

#ifdef __cplusplus
extern "C"
{
#endif


/* Offer one copied record to a registered chan of array of byte using real csendalt.
 * destination must have that exact Limbo static type; mover checks alone cannot prove it.
 * message is borrowed. Caller owns Dis execution/GC protection and an error boundary.
 * Never waits, never creates a channel buffer. NOT_FOUND means no receiver/space.
 * Dis allocation/VM errors propagate to the caller's kprotect boundary, not FVM status.
 * [GC:] The fresh array is consumed by csendalt on acceptance and rejection alike. */
FVM_Result dvm_ui_dis_offer(struct Channel *destination, const DVM_UIMessage *message);


/* Copy a received Dis byte-array frame into the UI-bound FlashVM queue without waiting.
 * bridge is live; frame is a rooted byte array, borrowed under Dis execution ownership.
 * NOT_FOUND retains caller ownership for retry. This does not receive from a Dis channel.
 * VM byte-array/type and wire validation errors return INVALID_ARGUMENT.
 * [GC:] No Dis allocation or managed pointer publication occurs in this function. */
FVM_Result dvm_ui_dis_export(DVM_UIBridge *bridge, struct Array *frame);


/* Borrowed, host-rooted ordinary Dis byte-array ports. Never pass this struct over a channel. */
typedef struct DVM_UIDisEndpoint
{
    struct Channel *requests; /* Registered chan of array of byte, read by the service. */
    struct Channel *replies;  /* Registered chan of array of byte, written by the service. */
} DVM_UIDisEndpoint;


/* Poll a real Dis channel without blocking its scheduler owner.
 * source has registered byte-array static type; out changes only after a valid frame.
 * Uses production irecv, restores active REG on success/error, and releases its received array.
 * Returns NOT_FOUND if not ready; malformed frames are consumed and return INVALID_ARGUMENT.
 * Caller retains Dis execution ownership and roots the source channel. */
FVM_Result dvm_ui_dis_take(struct Channel *source, DVM_UIMessage *out);


/* Pump-compatible protected ingress. opaque borrows DVM_UIDisEndpoint; message is copied on OK.
 * Converts caught Dis errors to ERROR and restores REG. Owner/roots stay with the host.
 * Returns NOT_FOUND for a busy service input, otherwise the adapter/validation status. */
FVM_Result dvm_ui_dis_pump_offer(void *opaque, const DVM_UIMessage *message);


/* Pump-compatible egress. opaque borrows DVM_UIDisEndpoint; out follows dvm_ui_dis_take.
 * No waits or heap reference escapes; returns the protected receive/decode status. */
FVM_Result dvm_ui_dis_pump_take(void *opaque, DVM_UIMessage *out);

#ifdef __cplusplus
}
#endif
#endif
