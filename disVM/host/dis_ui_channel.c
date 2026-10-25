/* Preserve production Dis rendezvous/ALT and reference movers at the UI boundary. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "disvm/dis_ui_channel.h"
#ifndef DVM_NATIVE_REGISTERS
#define DVM_R R /* Standalone fixture uses the original REG owner. */
#endif


/* Copy an envelope into a Dis-owned byte array and use the production channel mover.
 * destination/message and result/error ownership are specified in dis_ui_channel.h.
 * Host must serialize Dis heap/channel mutation; this is never a UI-thread call. */
FVM_Result dvm_ui_dis_offer(Channel *destination, const DVM_UIMessage *message)
{
    char storage[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX];
    FVM_Value value;
    FVM_Result result;
    Array *array;
    if (destination == nil || destination == H || destination->mover != movp)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = dvm_ui_encode(message, storage, sizeof(storage), &value);
    if (result != FVM_OK)
    {
        return result;
    }
    /* [GC:] heaparray owns a new reference; no fallible VM operation intervenes before transfer. */
    array = H2D(Array *, heaparray(&Tbyte, (int)fvm_value_string_size(&value)));
    memmove(array->data, storage, fvm_value_string_size(&value));
    /* [GC:] len=-1 forbids implicit buffering and freeptrs releases our temporary reference. */
    if (!csendalt(destination, &array, &Tptr, -1))
    {
        return FVM_NOT_FOUND;
    }
    return FVM_OK;
}


/* Validate byte-array geometry before borrowing its payload for a copied native send.
 * bridge/frame and return ownership are specified in dis_ui_channel.h.
 * Caller keeps frame rooted until return and retries it after transport backpressure. */
FVM_Result dvm_ui_dis_export(DVM_UIBridge *bridge, Array *frame)
{
    FVM_Value value;
    DVM_UIMessage message;
    FVM_Result result;
    if (!bridge || frame == nil || frame == H || frame->t != &Tbyte || frame->len < (int)DVM_UI_HEADER_SIZE ||
        frame->len > (int)(DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX) || frame->data == nil || frame->data == H)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = fvm_value_string_bytes(&value, (char *)frame->data, (size_t)frame->len);
    if (result != FVM_OK)
    {
        return result;
    }
    result = dvm_ui_decode(&value, &message);
    if (result != FVM_OK)
    {
        return result;
    }
    return dvm_ui_send(bridge, DVM_UI_DIS, &message, 0);
}


/* Protected call storage remains native and alive until kprotect returns. */
typedef struct DisAttempt
{
    Channel *channel;             /* Borrowed registered Dis channel. */
    const DVM_UIMessage *message; /* Borrowed outgoing record, or null during receive. */
    Array *array;                 /* [GC:] Received temporary reference, released before return. */
    FVM_Result result;            /* Adapter status published only after a complete offer. */
} DisAttempt;


/* Perform an offer inside an existing Dis exception boundary.
 * opaque borrows DisAttempt. Returns one after normal completion; Dis errors unwind kprotect. */
static int protected_offer(void *opaque)
{
    DisAttempt *attempt = (DisAttempt *)opaque;
    attempt->result = dvm_ui_dis_offer(attempt->channel, attempt->message);
    return 1;
}


/* Execute the production receive with an initialized pointer destination.
 * opaque borrows DisAttempt. Returns one normally; no wait is possible after owner readiness check. */
static int protected_take(void *opaque)
{
    DisAttempt *attempt = (DisAttempt *)opaque;
    DVM_R.s = &attempt->channel;
    DVM_R.d = &attempt->array;
    irecv();
    return 1;
}


/* Poll a registered byte-array channel and release the received reference after decoding.
 * source/out and exception/ownership behavior follow dis_ui_channel.h. */
FVM_Result dvm_ui_dis_take(Channel *source, DVM_UIMessage *out)
{
    DisAttempt attempt;
    REG saved;
    FVM_Value value;
    DVM_UIMessage decoded;
    FVM_Result result = FVM_ERROR;
    int completed;
    if (!out || source == nil || source == H || source->mover != movp || source->send == nil || source->recv == nil)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (source->send->prog == nil && source->size == 0)
    {
        return FVM_NOT_FOUND;
    }
    attempt.channel = source;
    attempt.message = nil;
    attempt.array = H;
    attempt.result = FVM_ERROR;
    saved = DVM_R;
    completed = kprotect(protected_take, &attempt);
    DVM_R = saved;
    if (completed && attempt.array != H && attempt.array != nil && attempt.array->t == &Tbyte &&
        attempt.array->len >= (int)DVM_UI_HEADER_SIZE &&
        attempt.array->len <= (int)(DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX) && attempt.array->data != nil &&
        attempt.array->data != H)
    {
        result = fvm_value_string_bytes(&value, (char *)attempt.array->data, (size_t)attempt.array->len);
        if (result == FVM_OK)
        {
            result = dvm_ui_decode(&value, &decoded);
        }
    }
    else if (completed)
    {
        result = FVM_INVALID_ARGUMENT;
    }
    /* [GC:] No Dis allocation occurs between receive and this release under the owner. */
    if (attempt.array != nil && attempt.array != H)
    {
        destroy(attempt.array);
    }
    if (result == FVM_OK)
    {
        *out = decoded;
    }
    return result;
}


/* Catch allocation/channel errors before returning through a native pump callback.
 * opaque/message and lifetime/result semantics follow dis_ui_channel.h. */
FVM_Result dvm_ui_dis_pump_offer(void *opaque, const DVM_UIMessage *message)
{
    DVM_UIDisEndpoint *endpoint = (DVM_UIDisEndpoint *)opaque;
    DisAttempt attempt;
    REG saved;
    int completed;
    if (!endpoint || !message || endpoint->requests == endpoint->replies)
    {
        return FVM_INVALID_ARGUMENT;
    }
    attempt.channel = endpoint->requests;
    attempt.message = message;
    attempt.array = H;
    attempt.result = FVM_ERROR;
    saved = DVM_R;
    completed = kprotect(protected_offer, &attempt);
    DVM_R = saved;
    return completed ? attempt.result : FVM_ERROR;
}


/* Route protected service egress through the same owner-held readiness/receive path.
 * opaque/out and lifetime/result semantics follow dis_ui_channel.h. */
FVM_Result dvm_ui_dis_pump_take(void *opaque, DVM_UIMessage *out)
{
    DVM_UIDisEndpoint *endpoint = (DVM_UIDisEndpoint *)opaque;
    if (!endpoint || endpoint->requests == endpoint->replies)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return dvm_ui_dis_take(endpoint->replies, out);
}
