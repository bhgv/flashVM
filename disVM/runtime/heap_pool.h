/* Private hosted arena owner; include lib9.h and pool.h before this header. */
#ifndef DVM_HEAP_POOL_H
#define DVM_HEAP_POOL_H
#include "channel_sync.h"
#include "disvm/heap_settings.h"


/* Stable arena and mutex; publish only after successful initialization. */
struct Pool
{
    Bhdr *arena; /* Owned arena/header, stable until quiescent destruction. */
    ulong bytes; /* Native checked extent, not a serialized VM field. */
    FVM_PortSync *sync; /* Protects allocation headers, not managed payload. */
};


/* Initialize a private zeroed pool. bytes must be aligned/bounded. Return0
 * unchanged on invalid input/native allocation failure; no callbacks/VM errors. */
int dvm_heap_pool_init(Pool *pool, ulong bytes);


/* Destroy only a quiescent pool with no allocated blocks. Return0 unchanged
 * on live/corrupt/uninitialized state. Caller joins/excludes every API user and
 * raw GC walker first; mutex cannot protect its own lifetime. */
int dvm_heap_pool_destroy(Pool *pool);
/* Allocate uninitialized bytes under the pool mutex without VM callbacks. */
void *dvm_heap_pool_try_alloc(Pool *pool, ulong size);
/* Query the generation of a caller-owned live allocation. Zero is invalid.
 * The caller pins the allocation throughout lookup; this is not a retain. */
DVM_HeapGeneration dvm_heap_pool_generation(Pool *pool, void *value);
#endif
