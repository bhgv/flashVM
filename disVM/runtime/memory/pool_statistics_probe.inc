/* Copyright (c) 2026 bhgv
 * EN: Independent actual-arena oracle and joined native allocator accounting tests.
 * RU: Независимый actual-arena oracle и joined native allocator accounting tests.
 */
#define MEMORY_WORKERS 8        /* Real OS owners of private allocations. */
#define MEMORY_ROUNDS 400       /* Native churn per owner, all allocations released. */

static Pool unit_pool;          /* Private arena joined before destruction, never managed objects. */
static unsigned int unit_ready; /* Atomic workers waiting at the start gate. */
static unsigned int unit_begin; /* Atomic parent-published start gate. */
static unsigned int unit_done;  /* Atomic successful worker completion count. */


/* EN/RU: Fail explicitly at semantic exit14; crash/timeouts never count.
 * Явно отказать с semantic exit14; crash/timeouts не считаются успехом. */
static void require_memory(int valid)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL coherent native pool accounting or ownership\n");
        exit(14);
    }
}


/* EN: Independently sum actual block headers while allocator mutation is excluded.
 * Compare the same instant's cached counters, never read managed payload or Type.
 * RU: Независимо сложить actual block headers при исключённой allocator mutation.
 * Сравнить cached counters того же instant; не читать managed payload либо Type. */
static void audit_unit_pool(void)
{
    Bhdr *block;
    uint64_t live = 0;
    uint64_t free_span = 0;
    uint64_t allocated = 0;
    uint64_t holes = 0;
    uchar *end = (uchar *)unit_pool.arena + unit_pool.bytes;
    fvm_port_sync_lock(unit_pool.sync);
    for (block = B2NB(unit_pool.arena); (uchar *)block < end; block = B2NB(block))
    {
        require_memory(block->size && !(block->size & 15UL) && block->size <= (ulong)(end - (uchar *)block));
        if (block->magic == MAGIC_F)
        {
            free_span += block->size;
            holes++;
        }
        else
        {
            require_memory(block->magic == MAGIC_A || block->magic == MAGIC_I);
            live += block->size;
            allocated++;
        }
    }
    require_memory((uchar *)block == end && live == unit_pool.statistics.live_span &&
                   free_span == unit_pool.statistics.free_span && allocated == unit_pool.statistics.live_blocks &&
                   holes == unit_pool.statistics.free_blocks &&
                   live + free_span + unit_pool.arena->size == unit_pool.bytes);
    fvm_port_sync_unlock(unit_pool.sync);
}


/* EN/RU: Own each allocated buffer through check/free; snapshots are native metadata only.
 * Владеть каждым buffer до check/free; snapshots содержат только native metadata. */
static void *memory_churn(void *argument)
{
    unsigned int round;
    unsigned int owner = (unsigned int)(uintptr_t)argument;
    __atomic_fetch_add(&unit_ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&unit_begin, __ATOMIC_ACQUIRE))
    {
        fvm_port_thread_sleep_us(50);
    }
    for (round = 0; round < MEMORY_ROUNDS; round++)
    {
        ulong size = 17 + ((round + owner) % 48) * 16;
        uchar *value = dvm_heap_pool_try_alloc(&unit_pool, size);
        require_memory(value && poolmsize(&unit_pool, value) >= size);
        memset(value, (int)owner, size);
        require_memory(value[0] == owner && value[size - 1] == owner);
        poolfree(&unit_pool, value);
    }
    __atomic_fetch_add(&unit_done, 1, __ATOMIC_RELEASE);
    return NULL;
}


/* EN/RU: Require every byte of a failed snapshot output to be zero.
 * Требовать нули во всех bytes output отказавшего snapshot. */
static void zero_memory_output(const DVM_HeapPoolStats *stats)
{
    const unsigned char *bytes = (const unsigned char *)stats;
    size_t index;
    for (index = 0; index < sizeof(*stats); index++)
    {
        require_memory(!bytes[index]);
    }
}


/* EN: Test actual split/reuse/free traffic with eight joined native owners, an
 * independent arena oracle, refusal zeroing, counter clamping and quiescent destroy.
 * RU: Проверить actual split/reuse/free traffic восьми joined native owners,
 * независимый arena oracle, refusal zeroing, counter clamping и quiescent destroy. */
static void memory_unit(void)
{
    pthread_t threads[MEMORY_WORKERS];
    DVM_HeapPoolStats stats;
    Pool empty = {0};
    Pool whole = {0};
    DVM_HeapPoolStats unchanged;
    void *all;
    unsigned int index;
    unsigned int observations = 0;
    require_memory(!__real_dvm_heap_pool_snapshot(&empty, &stats));
    zero_memory_output(&stats);
    require_memory(dvm_heap_pool_init(&whole, 65536));
    require_memory(__real_dvm_heap_pool_snapshot(&whole, &stats));
    all = dvm_heap_pool_try_alloc(&whole, (ulong)(stats.free_span - stats.block_overhead));
    require_memory(all && __real_dvm_heap_pool_snapshot(&whole, &stats) && !stats.free_span && !stats.free_blocks &&
                   stats.live_blocks == 1);
    unchanged = stats;
    require_memory(!dvm_heap_pool_try_alloc(&whole, 1));
    require_memory(__real_dvm_heap_pool_snapshot(&whole, &stats) && !memcmp(&unchanged, &stats, sizeof(stats)));
    require_memory(!dvm_heap_pool_destroy(&whole));
    require_memory(__real_dvm_heap_pool_snapshot(&whole, &stats) && !memcmp(&unchanged, &stats, sizeof(stats)));
    poolfree(&whole, all);
    require_memory(dvm_heap_pool_destroy(&whole));
    require_memory(dvm_heap_pool_init(&unit_pool, 8UL * 1024UL * 1024UL));
    for (index = 0; index < MEMORY_WORKERS; index++)
    {
        require_memory(!pthread_create(&threads[index], NULL, memory_churn, (void *)(uintptr_t)index));
    }
    while (__atomic_load_n(&unit_ready, __ATOMIC_ACQUIRE) != MEMORY_WORKERS)
    {
        fvm_port_thread_sleep_us(50);
    }
    __atomic_store_n(&unit_begin, 1, __ATOMIC_RELEASE);
    do
    {
        require_memory(__real_dvm_heap_pool_snapshot(&unit_pool, &stats));
        require_memory(stats.arena_bytes == stats.header_bytes + stats.live_span + stats.free_span);
        audit_unit_pool();
        observations++;
    } while (__atomic_load_n(&unit_done, __ATOMIC_ACQUIRE) != MEMORY_WORKERS);
    for (index = 0; index < MEMORY_WORKERS; index++)
    {
        require_memory(!pthread_join(threads[index], NULL));
    }
    require_memory(__real_dvm_heap_pool_snapshot(&unit_pool, &stats) && !stats.live_span && !stats.live_blocks &&
                   stats.allocations == MEMORY_WORKERS * MEMORY_ROUNDS && stats.releases == stats.allocations);
    audit_unit_pool();
    unit_pool.statistics.free_span++;
    memset(&stats, 0xa5, sizeof(stats));
    require_memory(!__real_dvm_heap_pool_snapshot(&unit_pool, &stats));
    zero_memory_output(&stats);
    unit_pool.statistics.free_span--;
    unit_pool.statistics.allocations = INT64_MAX - 1;
    unit_pool.statistics.releases = INT64_MAX - 1;
    for (index = 0; index < 3; index++)
    {
        void *value = dvm_heap_pool_try_alloc(&unit_pool, 32);
        require_memory(value != NULL);
        poolfree(&unit_pool, value);
    }
    require_memory(__real_dvm_heap_pool_snapshot(&unit_pool, &stats) && stats.allocations == INT64_MAX &&
                   stats.releases == INT64_MAX && stats.flags == 3 && !stats.live_blocks && !stats.live_span);
    require_memory(dvm_heap_pool_destroy(&unit_pool));
    require_memory(!__real_dvm_heap_pool_snapshot(&unit_pool, &stats));
    zero_memory_output(&stats);
    fprintf(stderr,
            "NATIVEMEMORYUNIT owners=8 joined=1 oracle=1 saturated=1 zeroed=1 whole=1 refused=1 observations=%u\n",
            observations);
}
