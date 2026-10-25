/* Caller-buffered incremental S-record reception; no transport, allocation or VM dependency. */
#ifndef FLASHVM_SSTREAM_H
#define FLASHVM_SSTREAM_H           /* Guard the C89 incremental decoder ABI. */
#include "srecord.h"
#define FVM_SSTREAM_MIN_CAPACITY 3U /* Smallest checksum + length + tag record. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Mutable native decoder for exactly one channel direction; fields are private policy state. */
typedef struct FVM_SStream
{
    size_t struct_size;    /* Exact native size filled by init. */
    unsigned char *buffer; /* Caller-owned disjoint receive storage, stable until disposal. */
    size_t capacity;       /* Accessible bytes in buffer. */
    size_t limit;          /* Body quota, 1..SRECORD_BODY_MAX. */
    size_t filled;         /* Bytes retained from current record. */
    size_t expected;       /* Total frame extent once canonical framing is known, otherwise zero. */
    int ready;             /* A verified borrowed view is live; push cannot overwrite it. */
    FVM_SResult error;     /* Latched parse/quota/checksum error until explicit reset. */
} FVM_SStream;


/* Initialize without touching buffer; capacity may be below limit and then enforces a tighter
     * bound. No allocation, locks, VM/GC or I/O. Output/storage must not overlap. */
FVM_API FVM_SResult fvm_sstream_init(FVM_SStream *, void *buffer, size_t capacity,
                                     size_t limit);


/* Consume at most one record, stopping before trailing input. Input must not overlap
     * buffer/state. used is consumed input even on MORE or a latched data error; unchanged on
     * invalid arguments. view changes only on OK and borrows buffer until reset/disposal.
     * Preserve/append only bytes not consumed; consumed bytes are already retained. Zero input
     * returns MORE for an incomplete record. After OK, push returns INVALID with used=0 until
     * reset; errors are latched with used=0 on retry. Bounds are enforced as soon as framing is
     * available, before copying an advertised body. */
FVM_API FVM_SResult fvm_sstream_push(FVM_SStream *, const void *, size_t size,
                                     FVM_SRecord *view, size_t *used);


/* Invalidate an outstanding view and discard prefix/error without wiping caller bytes.
     * Owner must quiesce users first; this does not resynchronize a transport or cancel remote
     * work. */
FVM_API FVM_SResult fvm_sstream_reset(FVM_SStream *);
#ifdef __cplusplus
}
#endif
#endif
