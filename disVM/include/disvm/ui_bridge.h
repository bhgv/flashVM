/* Shared value-only UI transport over the actual FlashVM channel implementation. */
#ifndef DISVM_UI_BRIDGE_H
#define DISVM_UI_BRIDGE_H /* Protect the C89-compatible bridge declarations. */
#include "flashvm/channel.h"
#include "flashvm/input.h"
#include "flashvm/keyboard.h"

#define DVM_UI_TEXT_MAX 1024U  /* Maximum payload bytes, excluding the fixed ASCII header. */
#define DVM_UI_PENDING_MAX 32U /* Buffered messages per direction; no unbounded input backlog. */
#define DVM_UI_HEADER_SIZE 53U /* Five prefix bytes and six eight-digit hexadecimal words. */
#define DVM_UI_UI 0U           /* Endpoint owned by the AS/SWF interface. */
#define DVM_UI_DIS 1U          /* Endpoint owned by the Dis service adapter. */

#ifdef __cplusplus
extern "C"
{
#endif


/* Operation classes; input events use id zero, requests/replies require a nonzero id. */
typedef enum DVM_UIKind
{
    DVM_UI_REQUEST = 1, /* Service request; code names an agreed operation. */
    DVM_UI_REPLY = 2,   /* Correlated service reply; code names an agreed status. */
    DVM_UI_POINTER = 3, /* Pointer position in twips; code is button, flags is phase. */
    DVM_UI_KEY = 4,     /* code is virtual key, flags is pressed, first is ASCII. */
    DVM_UI_TEXT = 5,    /* Committed text from FlashVM's host input path, not key synthesis. */
    DVM_UI_FOCUS = 6    /* flags is gained/lost focus; consumers reset held keys on loss. */
} DVM_UIKind;


/* Copied value record. No VM pointers, native handles or implicit heap ownership. */
typedef struct DVM_UIMessage
{
    unsigned long kind;         /* One DVM_UIKind value. */
    unsigned long id;           /* Unsigned 32-bit correlation ID; events use zero. */
    unsigned long code;         /* Unsigned 32-bit operation, status, button or key. */
    unsigned long flags;        /* Unsigned 32-bit class-specific flags. */
    long first;                 /* Signed 32-bit first argument or pointer X in twips. */
    long second;                /* Signed 32-bit second argument or pointer Y in twips. */
    size_t size;                /* Active Canonical UTF-8 payload bytes, including any NUL. */
    char text[DVM_UI_TEXT_MAX]; /* Owned bytes; no terminator is required or appended. */
} DVM_UIMessage;


typedef struct DVM_UIBridge DVM_UIBridge; /* Owns two independently directed FlashVM queues. */


/* Create paired queues. rendezvous is 0/1; out is untouched on failure.
 * Blocking sends in rendezvous mode complete only after receipt. Returns an FVM status. */
FVM_Result dvm_ui_create(int rendezvous, DVM_UIBridge **out);


/* Close both directions and wake waiting operations. bridge may be null.
 * Does not free storage; already queued buffered messages remain drainable. */
void dvm_ui_close(DVM_UIBridge *bridge);


/* Release both queues and bridge after all users/workers/wrappers have quiesced.
 * bridge may be null. Returns nothing; never destroy while an operation uses it. */
void dvm_ui_destroy(DVM_UIBridge *bridge);


/* Borrow this side's outgoing/incoming queues for AS wrappers or host adapters.
 * bridge/side/outgoing/incoming must be valid; outputs must be distinct. No retain.
 * Returns FVM_INVALID_ARGUMENT on invalid input, otherwise FVM_OK. */
FVM_Result dvm_ui_channels(DVM_UIBridge *bridge, unsigned int side, FVM_Channel **outgoing, FVM_Channel **incoming);


/* Encode a validated record as one portable counted string. storage has capacity bytes.
 * The returned value borrows storage. Failure leaves value untouched; no allocation.
 * first/second and header integers must fit 32 bits. Returns an FVM status. */
FVM_Result dvm_ui_encode(const DVM_UIMessage *message, char *storage, size_t capacity, FVM_Value *value);


/* Decode an unadorned counted string, validate header/payload, then publish out.
 * value is borrowed; out is unchanged on malformed/oversized input. Returns FVM status. */
FVM_Result dvm_ui_decode(const FVM_Value *value, DVM_UIMessage *out);


/* Send a copied record to the other side. blocking is 0/1; never block the UI owner.
 * Caller retains bridge/message throughout. FVM_NOT_FOUND means retry; no acceptance.
 * Queue receipt is transport acceptance, not execution or acknowledgement of a command. */
FVM_Result dvm_ui_send(DVM_UIBridge *bridge, unsigned int side, const DVM_UIMessage *message, int blocking);


/* Receive one owned queue message and decode it. blocking is 0/1.
 * Invalid messages are consumed/released; out stays unchanged on all failures.
 * Returns FVM_NOT_FOUND for empty/open, FVM_ERROR for closed/empty, or codec status. */
FVM_Result dvm_ui_receive(DVM_UIBridge *bridge, unsigned int side, DVM_UIMessage *out, int blocking);


/* Observe the already-dispatched FlashVM pointer and queue one event without waiting.
 * context is called on its owner thread. button/phase describe that dispatch.
 * Pixel coordinates become signed twips (truncation toward zero); no input is redispatched.
 * captured is required and receives the exact sample before send, including on backpressure.
 * On NOT_FOUND retain that record and retry dvm_ui_send, never recapture/replay input. */
FVM_Result dvm_ui_pointer(
    DVM_UIBridge *bridge, FVM_Context *context, FVM_MouseButton button, FVM_InputPhase phase, DVM_UIMessage *captured);


/* Observe FlashVM's published keyboard snapshot and queue a transition without waiting.
 * context is owner-thread only; code is the actual transition key, pressed is 0/1.
 * Reads ASCII only for matching key-down. Does not synthesize Unicode/IME text.
 * captured is required and receives the exact transition before send. On NOT_FOUND
 * retain it and retry dvm_ui_send. Capture validation errors leave captured unchanged. */
FVM_Result
dvm_ui_keyboard(DVM_UIBridge *bridge, FVM_Context *context, unsigned int code, int pressed, DVM_UIMessage *captured);

#ifdef __cplusplus
}
#endif
#endif
