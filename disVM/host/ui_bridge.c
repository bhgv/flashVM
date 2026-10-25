/* Value-only bridge: reuse FlashVM locking, waiters, cancellation and AS wrappers. */
#include "disvm/ui_bridge.h"
#include <stdlib.h>
#include <string.h>


/* Each direction has a distinct queue: a producer cannot receive its own message. */
struct DVM_UIBridge
{
    FVM_Channel *toward_dis; /* UI producer, Dis adapter consumer. */
    FVM_Channel *toward_ui;  /* Dis adapter producer, UI consumer. */
};


/* Admit canonical UTF-8 Unicode scalars so AS and Limbo cannot disagree through replacement.
 * text borrows size bytes, including NUL. Returns one for valid encoding, otherwise zero. */
static int valid_text(const char *text, size_t size)
{
    size_t i = 0;
    while (i < size)
    {
        unsigned long scalar;
        unsigned long minimum;
        unsigned int count;
        unsigned int c = (unsigned char)text[i++];
        if (c < 128)
        {
            continue;
        }
        if (c >= 194 && c <= 223)
        {
            scalar = c & 31U;
            minimum = 128;
            count = 1;
        }
        else if (c >= 224 && c <= 239)
        {
            scalar = c & 15U;
            minimum = 2048;
            count = 2;
        }
        else if (c >= 240 && c <= 244)
        {
            scalar = c & 7U;
            minimum = 65536;
            count = 3;
        }
        else
        {
            return 0;
        }
        if (size - i < count)
        {
            return 0;
        }
        while (count)
        {
            c = (unsigned char)text[i++];
            if ((c & 192U) != 128U)
            {
                return 0;
            }
            scalar = (scalar << 6) | (c & 63U);
            count--;
        }
        if (scalar < minimum || scalar > 0x10ffffUL || (scalar >= 0xd800UL && scalar <= 0xdfffUL))
        {
            return 0;
        }
    }
    return 1;
}


/* Validate portable scalar ranges before encoding or accepting an inbound record.
 * m is borrowed and nonnull. Returns one for an admitted common envelope, otherwise zero.
 * Operation-specific permission/schema checks remain with the receiving service. */
static int valid_message(const DVM_UIMessage *m)
{
    if (m->kind < DVM_UI_REQUEST || m->kind > DVM_UI_VIEW || m->id > 0xffffffffUL || m->code > 0xffffffffUL ||
        m->flags > 0xffffffffUL || m->first < (-2147483647L - 1L) || m->first > 2147483647L ||
        m->second < (-2147483647L - 1L) || m->second > 2147483647L || m->size > DVM_UI_TEXT_MAX)
    {
        return 0;
    }
    if (!valid_text(m->text, m->size))
    {
        return 0;
    }
    if (m->kind == DVM_UI_REQUEST || m->kind == DVM_UI_REPLY)
    {
        return m->id != 0;
    }
    return m->id == 0;
}


/* Write one unsigned 32-bit value as eight lowercase ASCII hexadecimal digits.
 * p points to eight writable bytes; value has already been range checked. Returns nothing. */
static void put_word(char *p, unsigned long value)
{
    static const char digits[] = "0123456789abcdef"; /* Locale-independent wire alphabet. */
    unsigned int i;
    for (i = 0; i < 8; i++)
    {
        p[7 - i] = digits[value & 15UL];
        value >>= 4;
    }
}


/* Parse eight lowercase hexadecimal digits. p borrows eight bytes, out receives a word.
 * Returns zero on malformed input; out is only written on success. */
static int get_word(const char *p, unsigned long *out)
{
    unsigned int i;
    unsigned long value = 0;
    for (i = 0; i < 8; i++)
    {
        unsigned int digit;
        if (p[i] >= '0' && p[i] <= '9')
        {
            digit = (unsigned int)(p[i] - '0');
        }
        else if (p[i] >= 'a' && p[i] <= 'f')
        {
            digit = (unsigned int)(p[i] - 'a' + 10);
        }
        else
        {
            return 0;
        }
        value = (value << 4) | digit;
    }
    *out = value;
    return 1;
}


/* Reconstruct an int32 value without implementation-defined unsigned-to-signed narrowing.
 * value is a parsed 32-bit word. Returns its signed two's-complement mathematical value. */
static long signed_word(unsigned long value)
{
    if (value <= 2147483647UL)
    {
        return (long)value;
    }
    return -1L - (long)(0xffffffffUL - value);
}


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


/* Build a canonical single-message frame; validate text with FlashVM's counted-string ABI.
 * Parameters/return follow ui_bridge.h. Output value is not published on failure. */
FVM_Result dvm_ui_encode(const DVM_UIMessage *message, char *storage, size_t capacity, FVM_Value *value)
{
    FVM_Value encoded;
    FVM_Result result;
    if (!message || !storage || !value || !valid_message(message) || capacity < DVM_UI_HEADER_SIZE + message->size)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memcpy(storage, "fvd1:", 5);
    put_word(storage + 5, message->kind);
    put_word(storage + 13, message->id);
    put_word(storage + 21, message->code);
    put_word(storage + 29, message->flags);
    put_word(storage + 37, ((unsigned long)message->first) & 0xffffffffUL);
    put_word(storage + 45, ((unsigned long)message->second) & 0xffffffffUL);
    memcpy(storage + DVM_UI_HEADER_SIZE, message->text, message->size);
    result = fvm_value_string_bytes(&encoded, storage, DVM_UI_HEADER_SIZE + message->size);
    if (result == FVM_OK)
    {
        *value = encoded;
    }
    return result;
}


/* Parse the common envelope before committing the output record.
 * Parameters/return follow ui_bridge.h; stack storage contains no managed references. */
FVM_Result dvm_ui_decode(const FVM_Value *value, DVM_UIMessage *out)
{
    DVM_UIMessage decoded;
    FVM_Value validated;
    unsigned long first;
    unsigned long second;
    size_t length;
    if (!value || !out || value->type != FVM_VALUE_STRING || value->metatable || !value->string)
    {
        return FVM_INVALID_ARGUMENT;
    }
    length = fvm_value_string_size(value);
    if (length < DVM_UI_HEADER_SIZE || length > DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX ||
        memcmp(value->string, "fvd1:", 5))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (fvm_value_string_bytes(&validated, value->string, length) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(&decoded, 0, sizeof(decoded));
    if (!get_word(value->string + 5, &decoded.kind) || !get_word(value->string + 13, &decoded.id) ||
        !get_word(value->string + 21, &decoded.code) || !get_word(value->string + 29, &decoded.flags) ||
        !get_word(value->string + 37, &first) || !get_word(value->string + 45, &second))
    {
        return FVM_INVALID_ARGUMENT;
    }
    decoded.first = signed_word(first);
    decoded.second = signed_word(second);
    decoded.size = length - DVM_UI_HEADER_SIZE;
    memcpy(decoded.text, value->string + DVM_UI_HEADER_SIZE, decoded.size);
    if (!valid_message(&decoded))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = decoded;
    return FVM_OK;
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
