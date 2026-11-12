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
    printf("PASS pool search live=%u initial_visits=%u total_visits=%u extras=%u\n", OBJECTS, initial, visits, extras);
    return 0;
}
