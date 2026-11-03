/* Exercise actual allocator metadata with native buffers, not managed objects. */
#include "lib9.h"
#include "pool.h"
#include "heap_pool.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WORKERS 8 /* Simultaneous independent owners of native allocations. */
#define ROUNDS 2000 /* Allocation/write/size/free rounds per owner. */

static Pool memory; /* Initialized before threads, destroyed after joins. */
static void *anchor; /* Stable live block used by reentrant fault callback. */
static __thread int depth; /* This thread's actual allocator mutex depth. */
static int faults; /* Serial-only callback count, never touched by workers. */
void __real_fvm_port_sync_lock(FVM_PortSync *sync);
void __real_fvm_port_sync_unlock(FVM_PortSync *sync);


/* Terminate at semantic10 on a failed invariant, including while locked.
 * message is borrowed; no allocation, cleanup, signals or VM errors involved. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        ssize_t result = write(2, message, strlen(message));
        (void)result;
        result = write(2, "\n", 1);
        (void)result;
        _Exit(10);
    }
}


/* Track actual port mutex acquisition; fixture has no additional mutexes. */
void __wrap_fvm_port_sync_lock(FVM_PortSync *sync)
{
    __real_fvm_port_sync_lock(sync);
    depth++;
}


/* Diagnostic state clears before releasing actual allocator exclusion. */
void __wrap_fvm_port_sync_unlock(FVM_PortSync *sync)
{
    check(depth == 1, "FAIL pool lock balance");
    depth--;
    __real_fvm_port_sync_unlock(sync);
}


/* Returning error callback must run outside allocator lock. Serial callback
 * reenters a valid pool size operation; invalid input never gains ownership. */
static void report(void *value, char *reason, ulong extra)
{
    (void)value;
    (void)reason;
    (void)extra;
    check(depth == 0, "FAIL pool callback under lock");
    faults++;
    check(poolmsize(&memory, anchor) >= 32, "FAIL pool callback reentry");
}

void (*poolfault)(void *, char *, ulong) = report; /* Serial fault oracle. */


/* id is a distinct borrowed native identity, alive through join. Exercise
 * allocation metadata concurrently; each payload belongs only to this thread. */
static void writer(void *id)
{
    unsigned int worker = *(unsigned int *)id;
    unsigned int round;
    for (round = 0; round < ROUNDS; round++)
    {
        ulong bytes = 32+(round*17+worker*31)%480;
        unsigned char pattern = (unsigned char)(round+worker);
        unsigned char *data = poolalloc(&memory, bytes);
        ulong index;
        check(data != nil, "FAIL pool allocation");
        memset(data, pattern, bytes);
        check(poolmsize(&memory, data) >= bytes, "FAIL pool size");
        if ((round & 63) == 0)
        {
            fvm_port_thread_sleep_us(1);
        }
        for (index = 0; index < bytes; index++)
        {
            check(data[index] == pattern, "FAIL pool payload overlap");
        }
        poolfree(&memory, data);
    }
}


/* No raw walk races a writer. Check all blocks and exact arena coverage after
 * joins; original header addresses remain usable by stopped-world collector. */
static void empty_chain(void)
{
    Bhdr *block;
    uchar *end = (uchar *)memory.arena+memory.bytes;
    for (block = B2NB(memory.arena); (uchar *)block < end; block = B2NB(block))
    {
        check(block->magic == MAGIC_F, "FAIL pool free publication");
        check(block->size >= 32 && !(block->size & 15UL) &&
            block->size <= (ulong)(end-(uchar *)block), "FAIL pool chain extent");
    }
    check((uchar *)block == end, "FAIL pool chain coverage");
}


/* Serial boundaries precede joined OS writers. argv --serial-only runs strict
 * controls before unsafe continuation. Return0 only on all explicit checks. */
int main(int argc, char **argv)
{
    FVM_PortThread *threads[WORKERS];
    unsigned int ids[WORKERS];
    unsigned int index;
    int invalid = 0;
    Bhdr *free_block;
    ulong saved_size;
    int serial = argc > 1 && strcmp(argv[1], "--serial-only") == 0;
    check(!dvm_heap_pool_init(&memory, 31), "FAIL pool bad extent");
    check(dvm_heap_pool_init(&memory, 8UL*1024UL*1024UL), "FAIL pool init");
    anchor = poolalloc(&memory, 32);
    check(anchor != nil, "FAIL pool anchor");
    check(B2NB(memory.arena)->magic == MAGIC_A, "FAIL pool allocation publication");
    check(!dvm_heap_pool_destroy(&memory), "FAIL pool live destroy");
    check(!poolalloc(&memory, ~0UL), "FAIL pool oversized refusal");
    poolfree(&memory, &invalid);
    check(poolmsize(&memory, &invalid) == 0 && faults == 2, "FAIL pool invalid refusal");
    free_block = B2NB(B2NB(memory.arena));
    saved_size = free_block->size;
    free_block->size = 0;
    check(!poolalloc(&memory, 32) && faults == 3, "FAIL pool corrupt refusal");
    free_block->size = saved_size;
    if (!serial)
    {
        for (index = 0; index < WORKERS; index++)
        {
            ids[index] = index;
            threads[index] = fvm_port_thread_start(writer, &ids[index]);
            check(threads[index] != nil, "FAIL pool thread start");
        }
        for (index = 0; index < WORKERS; index++)
        {
            fvm_port_thread_join(threads[index]);
        }
    }
    poolfree(&memory, anchor);
    empty_chain();
    check(dvm_heap_pool_destroy(&memory), "FAIL pool quiescent destroy");
    check(!memory.arena && !memory.sync && !memory.bytes, "FAIL pool destroyed state");
    puts(serial ? "PASS heap pool serial boundaries" : "PASS heap pool 8 OS owners, 16000 allocations");
    return 0;
}
