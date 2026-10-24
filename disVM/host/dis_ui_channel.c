/* Preserve production Dis rendezvous/ALT and reference movers at the UI boundary. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "disvm/dis_ui_channel.h"


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
