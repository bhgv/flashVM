/* Internal mapped-value transaction; caller supplies a validated live Type. */
#ifndef DVM_SHARED_MAPPED_H
#define DVM_SHARED_MAPPED_H
#include <stddef.h>

struct Type; /* Borrowed retained native descriptor, defined by interp.h. */


/* [GC:] Copy protected mapped record once, retaining incoming references under
 * cell/range guards and rooting displaced owners before publication. Native
 * preparation may fail before effects. Errors/destructors run only after unlock.
 * Caller independently owns live mapped source/destination and immutable Type;
 * size must be its already validated native extent. Requires ACTIVE serialized
 * worker admission, never grants parallel execution or mapping provenance.
 * Returns normally after retirement; VM errors propagate after native cleanup. */
void dvm_runtime_mapped_copy(const void *source, void *destination, struct Type *type, int size);
/* Move ownership from a disjoint, live ring slot. Clears source pointers
 * in the guarded publication. consumed stays set if retirement later raises. */
void dvm_runtime_mapped_consume(void *source, void *destination, struct Type *type,
    int size, int *consumed);


/* As for mapped_copy, with pre-effect BUSY replay for the decoded MOVMP. */
void dvm_runtime_mapped_move_opcode(const void *source, void *destination, struct Type *type, int size);


/* [GC:] Copy a validated count of elements as one transaction. Borrow mapped
 * spans and immutable type under NativeWorld. BUSY can replay before changes. */
void dvm_runtime_array_slice_copy(const void *source, void *destination, struct Type *type, size_t count);
/* [GC:] Transfer live ring spans using the supplied type. NULL consumed selects copy, otherwise
 * commit clears source pointers and sets consumed. Return zero on replay. */
int dvm_runtime_mapped_buffer_transfer(const void *source, void *destination, struct Type *type,
    int *consumed, int replay_opcode);
#endif
