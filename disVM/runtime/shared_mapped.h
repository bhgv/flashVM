/* Internal mapped-value transaction; caller supplies a validated live Type. */
#ifndef DVM_SHARED_MAPPED_H
#define DVM_SHARED_MAPPED_H

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
#endif
