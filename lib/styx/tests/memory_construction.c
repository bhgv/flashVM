/* Copyright (c) 2026 bhgv
 * EN: Inject each memory-pair construction failure; no half-owned endpoint or shared allocation may escape.
 * RU: Внедрять ошибки memory-pair construction; half-owned endpoint/shared allocation не должны остаться.
 */
#include "memory.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int stage;             /* 1 calloc, 2 mutex, 3 first adoption, 4 second adoption. */
static unsigned adoptions;    /* Actual calls to phycreate, successful or refused. */
static void *shared;          /* Only the shared Link uses calloc in this isolated constructor path. */
static unsigned released;     /* Number of tracked Link frees, not process-exit reclamation. */

extern void *__real_calloc(size_t, size_t);
extern void __real_free(void *);
extern int __real_pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
extern RemotePhy *__real_phycreate(const PhyOps *, void *, const PhyInfo *);


/* EN: Borrow allocation count/size; stage 1 fails before ownership exists.
 * Otherwise remember the actual allocation, forwarding native allocation unchanged.
 * RU: count/size — аргументы allocation; этап 1 отказывает до появления владения.
 * Иначе сохранить настоящий allocation, не изменяя его размер или allocator.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    if (stage == 1)
    {
        errno = ENOMEM;
        return NULL;
    }
    shared = __real_calloc(count, size);
    return shared;
}


/* EN/RU: Consume pointer through the real allocator; observe the shared Link's final release.
 * Поглотить pointer настоящим allocator; наблюдать final release shared Link.
 */
void __wrap_free(void *pointer)
{
    if (pointer && pointer == shared)
    {
        shared = NULL;
        released++;
    }
    __real_free(pointer);
}


/* EN/RU: Stage 2 refuses borrowed mutex/attributes without initialization; otherwise use the real API.
 * Этап 2 отказывает mutex/attributes без инициализации; иначе вызвать настоящий API.
 */
int __wrap_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attributes)
{
    return stage == 2 ? EAGAIN : __real_pthread_mutex_init(mutex, attributes);
}


/* EN: Refuse selected adoption without consuming state; ops/info remain borrowed.
 * All other calls use the real carrier constructor and its actual ownership contract.
 * RU: Отказать выбранному adoption без поглощения state; ops/info заимствованы.
 * Остальные вызовы идут настоящему carrier constructor с его настоящим контрактом владения.
 */
RemotePhy *__wrap_phycreate(const PhyOps *ops, void *state, const PhyInfo *info)
{
    adoptions++;
    if ((stage == 3 && adoptions == 1) || (stage == 4 && adoptions == 2))
    {
        return NULL;
    }
    return __real_phycreate(ops, state, info);
}


/* EN: No arguments. Run four failed constructions; return 1 if ownership/output
 * contracts fail. No test-created endpoint survives to process exit.
 * RU: Аргументов нет. Выполнить четыре неудачных construction; вернуть 1 при
 * нарушении ownership/output. Endpoints не оставляются до process exit.
 */
int main(void)
{
    for (stage = 1; stage <= 4; stage++)
    {
        adoptions = 0;
        released = 0;
        MemoryStats stats = {0};
        RemotePhy *left = (RemotePhy *)1; /* Raw sentinel is never dereferenced; valid output slots must become NULL. */
        RemotePhy *right = (RemotePhy *)1;
        int created = memory_pair(&left, &right, &stats, 0);
        unsigned expected_adoptions = stage < 3 ? 0 : stage == 3 ? 1 : 2;
        unsigned expected_destroyed = stage < 3 ? 0 : 2;
        if (created || left || right || shared || released != (unsigned)(stage != 1) ||
            adoptions != expected_adoptions || stats.destroyed != expected_destroyed)
        {
            fprintf(stderr, "memory construction stage %d: created=%d releases=%u adoptions=%u destroyed=%lu\n",
                    stage, created, released, adoptions, stats.destroyed);
            return 1;
        }
    }
    puts("PASS memory PHY construction: allocation, mutex and both adoption failures leave no owned state");
    return 0;
}
