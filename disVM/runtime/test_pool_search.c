/* Copyright (c) 2026 bhgv
 * EN: Bound real arena search work and preserve generation/address semantics.
 * RU: Ограничить настоящий поиск в арене и сохранить поколения/адреса блоков.
 */
#include "lib9.h"
#include "pool.h"
#include "heap_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJECTS 4096                      /* Live native buffers, large enough to expose repeated prefix scans. */
void (*poolfault)(void *, char *, ulong); /* No diagnostic callback installed in this native fixture. */
static unsigned visits;                   /* Private instrumentation counts actual inspected block headers. */
static unsigned limit;                    /* Deterministic work allowance, independent of elapsed time. */


/* EN: Fail explicitly with borrowed message; no managed heap or callback recovery.
 * RU: Завершиться явно с заимствованным message; управляемой кучи и восстановления нет. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL pool search: %s\n", message);
        exit(12);
    }
}


/* EN: Observe one real search visit in the private diagnostic build, with no
 * allocation or reentry while the allocator mutex is held. No production hook.
 * RU: Учесть настоящий шаг поиска в частной диагностической сборке без
 * выделения/повторного входа под мьютексом. В рабочем коде этого hook нет. */
void dvm_test_pool_search_visit(void)
{
    visits++;
    require(visits <= limit, "live prefix must not be rescanned for every allocation");
}


/* EN: Exhaust a private arena, then exercise allocation over adjacent holes
 * across the cursor. Verify separated holes, accounting and live identities.
 * No arguments or shared pool; destroy all owned allocations before return.
 * RU: Заполнить частную арену и проверить выделение на соседних свободных
 * блоках по обе стороны курсора, раздельные блоки, учёт и живые идентификаторы.
 * Аргументов и общего pool нет; снять все выделения перед возвратом. */
static void fragmented_arena(void)
{
    Pool pool = {0};
    DVM_HeapPoolStats before;
    DVM_HeapPoolStats after;
    require(dvm_heap_pool_init(&pool, 4096), "fragmented arena initialization");
    void *first = poolalloc(&pool, 128);
    void *second = poolalloc(&pool, 128);
    void *third = poolalloc(&pool, 128);
    unsigned char *separator = poolalloc(&pool, 128);
    void *isolated = poolalloc(&pool, 128);
    require(first && second && third && separator && isolated, "fragmented arena blocks");
    DVM_HeapKey old = dvm_heap_pool_key(&pool, first);
    DVM_HeapKey separator_key = dvm_heap_pool_key(&pool, separator);
    memset(separator, 0x5a, 128);
    require(dvm_heap_pool_snapshot(&pool, &before), "free tail accounting");
    void *tail = poolalloc(&pool, before.free_span - before.block_overhead);
    require(tail != NULL, "consume free tail");
    poolfree(&pool, first);
    poolfree(&pool, isolated);
    require(dvm_heap_pool_snapshot(&pool, &before) && before.free_blocks == 2,
            "separated hole accounting");
    require(!poolalloc(&pool, 256) && dvm_heap_pool_snapshot(&pool, &after) &&
            !memcmp(&before, &after, sizeof(before)), "separated holes cannot be merged");
    poolfree(&pool, third);
    poolfree(&pool, second);
    require(dvm_heap_pool_snapshot(&pool, &before) && before.free_blocks == 4,
            "adjacent hole accounting");
    void *merged = poolalloc(&pool, 256);
    require(merged == first && poolmsize(&pool, merged) >= 256, "reuse adjacent holes across cursor");
    memset(merged, 0xab, 256);
    require(!dvm_heap_pool_key_matches(&pool, merged, old) &&
            dvm_heap_pool_key_matches(&pool, separator, separator_key), "live and reused generations");
    for (unsigned index = 0; index < 128; index++)
    {
        require(separator[index] == 0x5a, "live payload survives adjacent merge");
    }
    require(dvm_heap_pool_snapshot(&pool, &after) && after.free_blocks == 2 &&
            after.live_blocks == before.live_blocks + 1 &&
            after.live_span - before.live_span == before.free_span - after.free_span &&
            after.allocations == before.allocations + 1 && after.releases == before.releases,
            "coalesced allocation accounting");
    void *remainder = poolalloc(&pool, 144);
    require(remainder != NULL, "reuse coalesced run remainder");
    require(poolalloc(&pool, 128) == isolated, "reuse isolated hole after merging");
    poolfree(&pool, isolated);
    poolfree(&pool, remainder);
    poolfree(&pool, merged);
    poolfree(&pool, separator);
    poolfree(&pool, tail);
    require(dvm_heap_pool_destroy(&pool), "fragmented arena cleanup");
}


/* EN: Keep thousands of actual native blocks live, then force wrap, reuse,
 * exhaustion and clean destruction. Validate payloads, generation changes and
 * stable block addresses. Native scratch and arena are owned through cleanup.
 * RU: Удержать тысячи настоящих native-блоков, проверить обход по кругу,
 * повторное использование, заполнение и очистку. Проверить данные, поколения
 * и стабильные адреса. Рабочие массивы и арена принадлежат тесту до удаления. */
int main(void)
{
    Pool memory = {0};
    void *objects[OBJECTS];
    DVM_HeapKey old;
    limit = OBJECTS * 8;
    require(dvm_heap_pool_init(&memory, 2UL * 1024 * 1024), "arena initialization");
    for (unsigned index = 0; index < OBJECTS; index++)
    {
        objects[index] = poolalloc(&memory, 32);
        require(objects[index] != NULL, "live native allocation");
        memset(objects[index], index & 255, 32);
    }
    unsigned initial = visits;
    old = dvm_heap_pool_key(&memory, objects[0]);
    poolfree(&memory, objects[0]);
    void *reused = poolalloc(&memory, 32);
    require(reused == objects[0] && !dvm_heap_pool_key_matches(&memory, reused, old),
            "freed block gets fresh generation");
    objects[0] = reused;
    memset(reused, 0, 32);
    for (unsigned index = 0; index < OBJECTS; index++)
    {
        require(((unsigned char *)objects[index])[0] == (index & 255), "stable live payload");
    }
    /* Consume the free tail exactly, then make the cursor wrap to an earlier hole. */
    Bhdr *tail = memory.cursor;
    require(tail->magic == MAGIC_A, "cursor follows immediately reused block");
    unsigned extras = 0;
    void *extra[OBJECTS * 8];
    limit = visits + OBJECTS * 32;
    while (extras < OBJECTS * 8 && (extra[extras] = dvm_heap_pool_try_alloc(&memory, 32)) != NULL)
    {
        extras++;
    }
    require(extras < OBJECTS * 8, "arena genuinely exhausted");
    poolfree(&memory, objects[OBJECTS / 2]);
    poolfree(&memory, objects[OBJECTS - 1]);
    void *last_hole = poolalloc(&memory, 32);
    require(last_hole == objects[OBJECTS - 1], "latest hole reused before wrap");
    objects[OBJECTS - 1] = last_hole;
    void *hole = poolalloc(&memory, 32);
    require(hole == objects[OBJECTS / 2], "wrapped arena reuses actual hole");
    objects[OBJECTS / 2] = hole;
    for (unsigned index = 0; index < extras; index++)
    {
        poolfree(&memory, extra[index]);
    }
    for (unsigned index = 0; index < OBJECTS; index++)
    {
        poolfree(&memory, objects[index]);
    }
    require(dvm_heap_pool_destroy(&memory) && !memory.cursor && !memory.arena, "quiescent arena cleanup");
    fragmented_arena();
    printf("PASS pool search live=%u initial_visits=%u total_visits=%u extras=%u\n", OBJECTS, initial, visits, extras);
    return 0;
}
