/* Serialize arena metadata without callbacks or VM errors under its mutex. */
#include "lib9.h"
#include "pool.h"
#include "heap_pool.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>


/* Native payload/header overhead, preserving retained collector layout. */
static ulong overhead(void)
{
    return (ulong)offsetof(Bhdr, u.data)+sizeof(Btail);
}


/* Validate one known chain position under pool exclusion. No callbacks.
 * block/end are already within the caller-owned arena; return metadata validity. */
static int valid_block(Bhdr *block, uchar *end)
{
    ulong remaining = (ulong)(end-(uchar *)block);
    return remaining >= offsetof(Bhdr, u.data) && block->size >= overhead() &&
        block->size <= remaining && !(block->size & 15UL) &&
        (block->magic == MAGIC_F || block->magic == MAGIC_A || block->magic == MAGIC_I);
}


/* Report only after releasing metadata exclusion. Hook may terminate/raise or
 * reenter a valid pool. A returning hook never authorizes the bad operation. */
static void fault(void *value, char *reason)
{
    if (poolfault)
    {
        poolfault(value, reason, 0);
    }
}


/* Allocate native resources before publishing pool fields. No shared users yet. */
int dvm_heap_pool_init(Pool *pool, ulong bytes)
{
    ulong header = (sizeof(Bhdr)+15UL)&~15UL;
    Bhdr *arena;
    Bhdr *block;
    FVM_PortSync *sync;
    if (!pool || pool->arena || pool->sync || pool->bytes || bytes > INT_MAX ||
        (bytes & 15UL) || bytes < header+sizeof(Bhdr)+16UL)
    {
        return 0;
    }
    arena = calloc(1, bytes);
    if (!arena)
    {
        return 0;
    }
    sync = fvm_port_sync_create();
    if (!sync)
    {
        free(arena);
        return 0;
    }
    arena->magic = MAGIC_E;
    arena->size = header;
    arena->csize = (int)bytes;
    arena->clink = nil;
    block = B2NB(arena);
    block->magic = MAGIC_F;
    block->size = bytes-header;
    pool->arena = arena;
    pool->bytes = bytes;
    pool->sync = sync;
    return 1;
}


/* First-fit split preserves all existing block boundaries/cursor addresses.
 * Return NULL on capacity refusal. Corruption reports outside mutex, then
 * returns NULL if callback returns. No managed/header initialization occurs here. */
void *poolalloc(Pool *pool, ulong size)
{
    Bhdr *block;
    Bhdr *remainder;
    uchar *end;
    ulong need;
    void *result = nil;
    int corrupt = 0;
    if (!pool || !pool->sync || size > pool->bytes-overhead()-16UL)
    {
        return nil;
    }
    need = (size+overhead()+15UL)&~15UL;
    fvm_port_sync_lock(pool->sync);
    end = (uchar *)pool->arena+pool->bytes;
    for (block = B2NB(pool->arena); (uchar *)block < end; block = B2NB(block))
    {
        if (!valid_block(block, end))
        {
            corrupt = 1;
            break;
        }
        if (block->magic != MAGIC_F || block->size < need)
        {
            continue;
        }
        if (block->size-need >= sizeof(Bhdr)+16UL)
        {
            remainder = (Bhdr *)((uchar *)block+need);
            remainder->magic = MAGIC_F;
            remainder->size = block->size-need;
            block->size = need;
        }
        block->magic = MAGIC_A;
        result = B2D(block);
        break;
    }
    fvm_port_sync_unlock(pool->sync);
    if (corrupt)
    {
        fault(pool, "alloc:block");
    }
    return result;
}


/* Check numeric range before reading a candidate header. Caller independently
 * owns the allocation; these checks do not establish pointer provenance/lifetime.
 * Called only with metadata mutex held; no callbacks. */
static Bhdr *owned_block(Pool *pool, void *value)
{
    uintptr_t address = (uintptr_t)value;
    uintptr_t first = (uintptr_t)B2NB(pool->arena)+offsetof(Bhdr, u.data);
    uintptr_t limit = (uintptr_t)pool->arena+pool->bytes;
    Bhdr *block;
    if (address < first || address > limit-overhead() ||
        ((address-(uintptr_t)pool->arena-offsetof(Bhdr, u.data)) & 15UL) != 0)
    {
        return nil;
    }
    block = (Bhdr *)(address-offsetof(Bhdr, u.data));
    if (!valid_block(block, (uchar *)limit) ||
        (block->magic != MAGIC_A && block->magic != MAGIC_I))
    {
        return nil;
    }
    return block;
}


/* Consume a caller-owned block. Invalid input faults only after unlock; a
 * returning callback leaves metadata unchanged. Never runs payload destruction. */
void poolfree(Pool *pool, void *value)
{
    Bhdr *block;
    if (!pool || !pool->sync)
    {
        fault(value, "alloc:D2B");
        return;
    }
    fvm_port_sync_lock(pool->sync);
    block = owned_block(pool, value);
    if (block)
    {
        block->magic = MAGIC_F;
    }
    fvm_port_sync_unlock(pool->sync);
    if (!block)
    {
        fault(value, "alloc:D2B");
    }
}


/* Borrow stable arena header. Its raw block walk needs stopped-world exclusion
 * or complete caller quiescence; this return does not carry the metadata lock. */
Bhdr *poolchain(Pool *pool)
{
    return pool->arena;
}


/* Observe size of caller-owned live storage under metadata mutex. No payload
 * pin/COW right granted. Invalid input reports outside lock and returns0. */
ulong poolmsize(Pool *pool, void *value)
{
    Bhdr *block;
    ulong result = 0;
    if (!pool || !pool->sync)
    {
        fault(value, "alloc:D2B");
        return 0;
    }
    fvm_port_sync_lock(pool->sync);
    block = owned_block(pool, value);
    if (block)
    {
        result = block->size-overhead();
    }
    fvm_port_sync_unlock(pool->sync);
    if (!block)
    {
        fault(value, "alloc:D2B");
    }
    return result;
}


/* Quiescent destruction refuses live/corrupt blocks without changing pool.
 * It is not a concurrent close operation. Native frees happen after unlock. */
int dvm_heap_pool_destroy(Pool *pool)
{
    Bhdr *block;
    Bhdr *arena;
    FVM_PortSync *sync;
    uchar *end;
    if (!pool || !pool->sync)
    {
        return 0;
    }
    sync = pool->sync;
    fvm_port_sync_lock(sync);
    end = (uchar *)pool->arena+pool->bytes;
    for (block = B2NB(pool->arena); (uchar *)block < end; block = B2NB(block))
    {
        if (!valid_block(block, end) || block->magic != MAGIC_F)
        {
            fvm_port_sync_unlock(sync);
            return 0;
        }
    }
    arena = pool->arena;
    pool->arena = nil;
    pool->sync = nil;
    pool->bytes = 0;
    fvm_port_sync_unlock(sync);
    fvm_port_sync_destroy(sync);
    free(arena);
    return 1;
}
