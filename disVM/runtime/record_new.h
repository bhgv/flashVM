/* Guarded record construction for the hosted NEW/NEWZ instructions. */
#ifndef DVM_RECORD_NEW_H
#define DVM_RECORD_NEW_H

struct Type; /* Immutable live descriptor owned by the caller. */


/* [GC:] Construct a fresh record and publish into a rooted owning destination.
 * clear selects scalar zeroing. Both modes initialize mapped fields to H.
 * Caller retains frame/cell/Type lifetime and ACTIVE NativeWorld admission.
 * Monitor may inspect or raise after rooting, and may write application slots
 * through the normal guarded APIs. It cannot retain the private candidate or
 * leave admission. Errors propagate after consuming cleanup. */
void dvm_runtime_record_new(void *destination, struct Type *type, int clear);


/* [GC:] Same publication contract for an owning array. count is a full native
 * WORD. Each element gets H in mapped slots, with optional scalar zeroing. */
void dvm_runtime_array_new(void *destination, struct Type *type, WORD count, int clear);


/* [GC:] Publish a complete channel and its optional buffer. The caller owns
 * the payload descriptor for buffered or mapped channels. */
Channel *dvm_runtime_channel_new(void *destination, struct Type *type, WORD count,
    void (*mover)(void), WORD message_size);
void dvm_runtime_raw_channel_new(void *destination, WORD count, WORD size,
    void (*mover)(void));
#endif
