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


/* EN: Read the type index or fill an empty slot from an existing SHARED dispatch.
 * RU: Прочитать индекс типа или заполнить пустую ячейку из текущего SHARED.
 * Zero leaves the destination unchanged and requests the admitted fallback. */
int dvm_runtime_record_shared_index(const void *source, WORD *index);
int dvm_runtime_record_try_shared(void *destination, struct Type *type, int clear);

/* EN: Read array operands and publish an empty-slot array in SHARED.
 * RU: Прочитать операнды массива и опубликовать его в пустую ячейку из SHARED. */
int dvm_runtime_array_shared_operands(const void *source, const void *middle, WORD *count, WORD *index);
int dvm_runtime_array_try_shared(void *destination, struct Type *type, WORD count, int clear);

/* [GC:] Same publication contract for an owning array. count is a full native
 * WORD. Each element gets H in mapped slots, with optional scalar zeroing. */
void dvm_runtime_array_new(void *destination, struct Type *type, WORD count, int clear);


/* [GC:] Publish a complete channel and its optional buffer. The caller owns
 * the payload descriptor for buffered or mapped channels. */
Channel *dvm_runtime_channel_new(void *destination, struct Type *type, WORD count,
    void (*mover)(void), WORD message_size);
/* EN: Publish a typed channel directly into an empty shared slot.
 * RU: Опубликовать типизированный канал в пустую ячейку из SHARED. */
Channel *dvm_runtime_channel_try_shared(void *destination, struct Type *type,
    WORD count, void (*mover)(void));

void dvm_runtime_raw_channel_new(void *destination, WORD count, WORD size,
    void (*mover)(void));
#endif
