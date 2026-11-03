/* Six-stripe native transaction for three small independently live byte spans. */
#ifndef DVM_SMALL_RANGE_H
#define DVM_SMALL_RANGE_H
#include "disvm/cell_access.h"
#ifdef __cplusplus
extern "C"
{
#endif


/* Zero before begin, stable noncopyable owner-local storage through end. */
typedef struct DVM_TripleAccess
{
    DVM_CellDomain *domain; /* Borrowed existing domain, pinned until end/rollback. */
    DVM_NativeContext *context; /* Exact bound owner while acquired. */
    DVM_ObjectGuard *guards[6]; /* Sorted unique stripes, not managed data. */
    unsigned int count; /* Selected unique stripe count. */
    unsigned int held; /* Acquired prefix, private rollback metadata. */
} DVM_TripleAccess;


/* Claim source/second/destination spans of 1..sizeof(void*) in one ordered union.
 * Caller keeps mapped spans and canonical domain independently alive. Zero access
 * and unguarded bound context required; no native allocation or owner wait.
 * Return ACQUIRED, BUSY or REFUSED; partial claims/borrow roll back on refusal.
 * Metadata mutexes may wait. No VM callback, retain/root or provenance proof.
 * Never hold across waits, VM/errors or arbitrary OS/native unwinding. */
DVM_CellResult dvm_small_range_access_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, const void *second, size_t second_size,
    void *destination, size_t destination_size, DVM_TripleAccess *access);


/* End current-owner access, release reverse prefix and borrow, zero metadata.
 * Return zero unchanged on foreign/depth/state refusal. Caller owns domain/spans
 * through return. No allocation, payload read, callback or VM transition. */
int dvm_small_range_access_end(DVM_TripleAccess *access);
#ifdef __cplusplus
}
#endif
#endif
