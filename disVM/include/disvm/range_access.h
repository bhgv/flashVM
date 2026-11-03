/* Ordered native byte-span exclusion sharing the existing pointer-cell domain. */
#ifndef DVM_RANGE_ACCESS_H
#define DVM_RANGE_ACCESS_H
#include "disvm/cell_access.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define DVM_RANGE_GUARDS 256 /* Stripe capacity, never an object or thread quota. */


/* Zeroed stable noncopyable owner-local storage; never modify live metadata. */
typedef struct DVM_RangeAccess
{
    DVM_CellDomain *domain; /* Borrowed through end/partial rollback. */
    DVM_NativeContext *context; /* Exact bound owner. */
    DVM_ObjectGuard *guards[DVM_RANGE_GUARDS]; /* Sorted unique selected stripes. */
    unsigned int count; /* Selected stripe count, including zero for empty spans. */
    unsigned int held; /* Acquired prefix, implementation state only. */
} DVM_RangeAccess;


/* Claim complete arbitrary-sized source/destination byte spans, without native
 * allocation or object-owner waits. Same domain protects overlapping pointer
 * and byte access. Empty spans ignore their address and select no stripe.
 * Large spans select all stripes; collisions serialize unrelated accesses.
 * Caller owns mapped ranges/domain lifetime and supplies zero access storage.
 * Wrong geometry/role/BUSY rolls back guards and borrow; invalid nonzero access
 * refuses unchanged. Metadata locks may wait. No VM/managed ownership callback,
 * root acquisition, mapping proof or permission for unguarded competitors. */
DVM_CellResult dvm_range_access_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, void *destination, size_t destination_size,
    DVM_RangeAccess *access);


/* End exact current-owner access, including a successfully acquired empty set.
 * Return0 unchanged for wrong owner/depth/state. Caller retains domain/storage;
 * no callback, allocation, payload read or VM transition. Do not hold access
 * across waits, error/VM callbacks or arbitrary OS/native unwinding. */
int dvm_range_access_end(DVM_RangeAccess *access);
#ifdef __cplusplus
}
#endif
#endif
