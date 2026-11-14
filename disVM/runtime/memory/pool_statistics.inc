/* Copyright (c) 2026 bhgv
 * EN: O(1) accounting under the allocator's existing metadata mutex.
 * RU: Учёт O(1) под существующим mutex метаданных аллокатора.
 */


/* EN/RU: Increment one owned history counter up to signed-big max; latch its flag.
 * Увеличить собственный history counter до signed-big max; зафиксировать flag. */
static void increment_pool_history(uint64_t *counter, unsigned int *flags, unsigned int bit)
{
    if (*counter < INT64_MAX)
    {
        (*counter)++;
    }
    else
    {
        *flags |= bit;
    }
}


/* EN: Record one allocated span after split. Caller holds pool mutex; split means
 * a free remainder survives, otherwise one free block is consumed. No callbacks.
 * RU: Учесть allocated span после split. Caller держит pool mutex; split означает
 * сохранённый free remainder, иначе поглощается один free block. Без callbacks. */
static void record_pool_allocation(Pool *pool, ulong span, int split)
{
    DVM_HeapPoolStats *stats = &pool->statistics;
    stats->live_span += span;
    stats->free_span -= span;
    stats->live_blocks++;
    if (!split)
    {
        stats->free_blocks--;
    }
    if (stats->live_span > stats->peak_live_span)
    {
        stats->peak_live_span = stats->live_span;
    }
    increment_pool_history(&stats->allocations, &stats->flags, DVM_POOL_ALLOCATIONS_SATURATED);
}


/* EN/RU: Record one valid free span under pool mutex; no coalescing/callbacks.
 * Учесть один valid free span под pool mutex; без coalescing/callbacks. */
static void record_pool_release(Pool *pool, ulong span)
{
    DVM_HeapPoolStats *stats = &pool->statistics;
    stats->live_span -= span;
    stats->free_span += span;
    stats->live_blocks--;
    stats->free_blocks++;
    increment_pool_history(&stats->releases, &stats->flags, DVM_POOL_RELEASES_SATURATED);
}


/* EN: Validate bounded accounting, not native object/arena provenance. stats are
 * borrowed immutable under pool mutex. Refuse impossible partition/history.
 * RU: Проверить bounded accounting, не native object/arena provenance. stats
 * неизменяемы под pool mutex. Отказать невозможным partition/history. */
static int valid_pool_statistics(const Pool *pool, const DVM_HeapPoolStats *stats)
{
    uint64_t area;
    if (stats->arena_bytes != pool->bytes || stats->arena_bytes > INT64_MAX ||
        stats->header_bytes != pool->arena->size || stats->header_bytes >= stats->arena_bytes ||
        stats->block_overhead != overhead() || !stats->block_overhead || stats->allocations > INT64_MAX ||
        stats->releases > INT64_MAX || (stats->flags & ~(DVM_POOL_ALLOCATIONS_SATURATED | DVM_POOL_RELEASES_SATURATED)))
    {
        return 0;
    }
    area = stats->arena_bytes - stats->header_bytes;
    if (stats->live_span > area || stats->free_span != area - stats->live_span ||
        stats->peak_live_span < stats->live_span || stats->peak_live_span > area ||
        stats->live_blocks > stats->live_span / stats->block_overhead ||
        stats->free_blocks > stats->free_span / stats->block_overhead || (!stats->live_blocks && stats->live_span) ||
        (!stats->free_blocks && stats->free_span))
    {
        return 0;
    }
    return stats->flags ||
           (stats->allocations >= stats->releases && stats->allocations - stats->releases == stats->live_blocks);
}


/* EN/RU: Copy one coherent accounting record under existing pool lock; failure is zero.
 * Копировать coherent accounting record под existing pool lock; отказ даёт нули. */
int dvm_heap_pool_snapshot(Pool *pool, DVM_HeapPoolStats *output)
{
    int valid = 0;
    if (!output)
    {
        return 0;
    }
    memset(output, 0, sizeof(*output));
    if (!pool || !pool->sync)
    {
        return 0;
    }
    fvm_port_sync_lock(pool->sync);
    if (pool->arena && valid_pool_statistics(pool, &pool->statistics))
    {
        *output = pool->statistics;
        valid = 1;
    }
    fvm_port_sync_unlock(pool->sync);
    return valid;
}
