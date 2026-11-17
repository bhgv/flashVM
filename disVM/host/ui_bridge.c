/* Copyright (c) 2026 bhgv
 * EN: Local copied-value channels delegate fvd1 serialization to the portable codec.
 * RU: Локальные каналы значений используют общий переносимый кодек fvd1.
 */
/* Value-only bridge: reuse FlashVM locking, waiters, cancellation and AS wrappers. */
#include "disvm/ui_bridge.h"
#include "disvm/ui_frame.h"
#include <stdlib.h>
#include <string.h>


/* Each direction has a distinct queue: a producer cannot receive its own message. */
struct DVM_UIBridge
{
    FVM_Channel *toward_dis; /* UI producer, Dis adapter consumer. */
    FVM_Channel *toward_ui;  /* Dis adapter producer, UI consumer. */
};


/* Allocate and initialize both directions; unwind every partial allocation on failure.
 * rendezvous is 0/1, out is nonnull. Returns status and publishes out only after success. */
FVM_Result dvm_ui_create(int rendezvous, DVM_UIBridge **out)
{
    DVM_UIBridge *bridge;
    FVM_Result result;
    if (!out || (rendezvous != 0 && rendezvous != 1))
    {
        return FVM_INVALID_ARGUMENT;
    }
    bridge = (DVM_UIBridge *)calloc(1, sizeof(*bridge));
    if (!bridge)
    {
        return FVM_OUT_OF_MEMORY;
    }
    result = fvm_channel_create(&bridge->toward_dis);
    if (result == FVM_OK)
    {
        result = fvm_channel_create(&bridge->toward_ui);
    }
    if (result == FVM_OK)
    {
        result = fvm_channel_set_capacity(bridge->toward_dis, DVM_UI_PENDING_MAX);
    }
    if (result == FVM_OK)
    {
        result = fvm_channel_set_capacity(bridge->toward_ui, DVM_UI_PENDING_MAX);
    }
    if (result == FVM_OK && rendezvous)
    {
        result = fvm_channel_set_rendezvous(bridge->toward_dis, 1);
        if (result == FVM_OK)
        {
            result = fvm_channel_set_rendezvous(bridge->toward_ui, 1);
        }
    }
    if (result != FVM_OK)
    {
        dvm_ui_destroy(bridge);
        return result;
    }
    *out = bridge;
    return FVM_OK;
}


/* Request shutdown on both queues; bridge is optional and storage remains owned.
 * Returns nothing. Callers must subsequently join workers before destruction. */
void dvm_ui_close(DVM_UIBridge *bridge)
{
    if (bridge)
    {
        if (bridge->toward_dis)
        {
            fvm_channel_close(bridge->toward_dis);
        }
        if (bridge->toward_ui)
        {
            fvm_channel_close(bridge->toward_ui);
        }
    }
}


/* Free a quiescent bridge, including partial construction. bridge may be null.
 * Returns nothing; live worker operations must have ended before entry. */
void dvm_ui_destroy(DVM_UIBridge *bridge)
{
    if (bridge)
    {
        if (bridge->toward_dis)
        {
            fvm_channel_release(bridge->toward_dis);
        }
        if (bridge->toward_ui)
        {
            fvm_channel_release(bridge->toward_ui);
        }
        free(bridge);
    }
}


/* Resolve directional borrowed handles; reject aliases before publishing either output.
 * Parameters/return follow ui_bridge.h; this operation allocates nothing and never waits. */
FVM_Result dvm_ui_channels(DVM_UIBridge *bridge, unsigned int side, FVM_Channel **outgoing, FVM_Channel **incoming)
{
    if (!bridge || side > DVM_UI_DIS || !outgoing || !incoming || outgoing == incoming)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *outgoing = side == DVM_UI_UI ? bridge->toward_dis : bridge->toward_ui;
    *incoming = side == DVM_UI_UI ? bridge->toward_ui : bridge->toward_dis;
    return FVM_OK;
}


/* EN/RU: Preserve the public counted-value ABI, delegating the portable frame to the shared codec.
 * Message/storage/value borrowed, output unchanged on failure. Сохранить counted-value ABI,
 * portable frame обрабатывается общим codec, inputs borrowed, output неизменен при отказе. */
FVM_Result dvm_ui_encode(const DVM_UIMessage *message, char *storage, size_t capacity, FVM_Value *value)
{
    FVM_Value encoded;
    FVM_Result result;
    size_t size;
    if (!value)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = dvm_ui_frame_encode(message, storage, capacity, &size);
    if (result == FVM_OK)
    {
        result = fvm_value_string_bytes(&encoded, storage, size);
        if (result == FVM_OK)
        {
            *value = encoded;
        }
    }
    return result;
}


/* EN/RU: Validate the public value capability before decoding the same portable bytes.
 * Borrow value/out, output only on success. Проверить value capability и общий frame;
 * value/out borrowed, output только при успехе. */
FVM_Result dvm_ui_decode(const FVM_Value *value, DVM_UIMessage *out)
{
    FVM_Value validated;
    size_t length;
    if (!value || !out || value->type != FVM_VALUE_STRING || value->metatable || !value->string)
    {
        return FVM_INVALID_ARGUMENT;
    }
    length = fvm_value_string_size(value);
    if (length < DVM_UI_HEADER_SIZE || length > DVM_UI_HEADER_SIZE+DVM_UI_TEXT_MAX ||
        fvm_value_string_bytes(&validated, value->string, length) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return dvm_ui_frame_decode(value->string, length, out);
}


/* Encode and copy to this side's outgoing queue; never hold VM pointers across waiting.
 * Parameters/return follow ui_bridge.h; blocking must be 0/1 and UI uses zero. */
FVM_Result dvm_ui_send(DVM_UIBridge *bridge, unsigned int side, const DVM_UIMessage *message, int blocking)
{
    FVM_Channel *outgoing;
    FVM_Channel *incoming;
    FVM_Value value;
    char storage[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX];
    FVM_Result result;
    if (blocking != 0 && blocking != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = dvm_ui_channels(bridge, side, &outgoing, &incoming);
    if (result != FVM_OK)
    {
        return result;
    }
    result = dvm_ui_encode(message, storage, sizeof(storage), &value);
    if (result != FVM_OK)
    {
        return result;
    }
    return blocking ? fvm_channel_send(outgoing, &value) : fvm_channel_try_send(outgoing, &value);
}


/* Consume an independently owned string so another receiver cannot invalidate decoding.
 * Parameters/return follow ui_bridge.h; every successfully received message is released. */
FVM_Result dvm_ui_receive(DVM_UIBridge *bridge, unsigned int side, DVM_UIMessage *out, int blocking)
{
    FVM_Channel *outgoing;
    FVM_Channel *incoming;
    FVM_Value value;
    FVM_Result result;
    if (!out || (blocking != 0 && blocking != 1))
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = dvm_ui_channels(bridge, side, &outgoing, &incoming);
    if (result != FVM_OK)
    {
        return result;
    }
    result = fvm_channel_receive_owned(incoming, &value, blocking);
    if (result != FVM_OK)
    {
        return result;
    }
    result = dvm_ui_decode(&value, out);
    fvm_channel_message_release(&value);
    return result;
}


/* Capture FlashVM pointer state after its normal dispatch, preserving the UI owner rule.
 * Parameters/return follow ui_bridge.h. Queue pressure never redispatches input. */
FVM_Result
dvm_ui_capture_pointer(FVM_Context *context, FVM_MouseButton button, FVM_InputPhase phase, DVM_UIMessage *captured)
{
    DVM_UIMessage message;
    double x;
    double y;
    FVM_Result result;
    if (!context || !captured || button < FVM_MOUSE_NONE || button > FVM_MOUSE_WHEEL_DOWN || phase < FVM_INPUT_BEGIN ||
        phase > FVM_INPUT_END)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = fvm_input_get_pointer(context, &x, &y);
    if (result != FVM_OK)
    {
        return result;
    }
    x *= 20.0;
    y *= 20.0;
    if (!(x >= -2147483648.0 && x <= 2147483647.0 && y >= -2147483648.0 && y <= 2147483647.0))
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(&message, 0, sizeof(message));
    message.kind = DVM_UI_POINTER;
    message.code = (unsigned long)button;
    message.flags = (unsigned long)phase;
    message.first = (long)x;
    message.second = (long)y;
    *captured = message;
    return FVM_OK;
}


/* Capture an existing FlashVM keyboard snapshot, without installing a second input stack.
 * Parameters/return follow ui_bridge.h; Unicode composition is carried as DVM_UI_TEXT. */
FVM_Result dvm_ui_capture_keyboard(FVM_Context *context, unsigned int code, int pressed, DVM_UIMessage *captured)
{
    DVM_UIMessage message;
    FVM_KeyboardState keyboard;
    FVM_Result result;
    if (!context || !captured || code < FVM_KEYBOARD_FIRST_CODE || code >= FVM_KEYBOARD_CODE_COUNT ||
        (pressed != 0 && pressed != 1))
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.struct_size = sizeof(keyboard);
    result = fvm_input_get_keyboard(context, &keyboard);
    if (result != FVM_OK)
    {
        return result;
    }
    if ((int)keyboard.down[code] != pressed)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(&message, 0, sizeof(message));
    message.kind = DVM_UI_KEY;
    message.code = code;
    message.flags = (unsigned long)pressed;
    if (pressed && keyboard.last_code == code)
    {
        message.first = (long)keyboard.last_ascii;
    }
    *captured = message;
    return FVM_OK;
}


/* Capture the exact pointer sample, then attempt the original nonblocking direct send.
 * bridge/context/button/phase/captured and return ownership follow ui_bridge.h. */
FVM_Result dvm_ui_pointer(
    DVM_UIBridge *bridge, FVM_Context *context, FVM_MouseButton button, FVM_InputPhase phase, DVM_UIMessage *captured)
{
    FVM_Result result;
    if (!bridge)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = dvm_ui_capture_pointer(context, button, phase, captured);
    return result == FVM_OK ? dvm_ui_send(bridge, DVM_UI_UI, captured, 0) : result;
}


/* Capture without redispatching, then attempt the original keyboard send.
 * bridge/context/code/pressed/captured and return ownership follow ui_bridge.h. */
FVM_Result
dvm_ui_keyboard(DVM_UIBridge *bridge, FVM_Context *context, unsigned int code, int pressed, DVM_UIMessage *captured)
{
    FVM_Result result;
    if (!bridge)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = dvm_ui_capture_keyboard(context, code, pressed, captured);
    return result == FVM_OK ? dvm_ui_send(bridge, DVM_UI_UI, captured, 0) : result;
}
