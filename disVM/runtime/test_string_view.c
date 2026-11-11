/* Copyright (c) 2026 bhgv
 * EN: Observe real Unicode cache allocation, publication and collector freeing.
 * RU: Наблюдать настоящее выделение, публикацию и освобождение Unicode-кэша.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "disvm/native_context.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t ledger = PTHREAD_MUTEX_INITIALIZER; /* Observer metadata only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Bounded first-allocation rendezvous. */
static _Thread_local int observing; /* Only the real shared-string Sys call. */
static _Thread_local size_t expected_size; /* Raw UTF-8 cache allocation extent. */
static String *shared; /* Borrowed shared identity, protected by actual VM owners. */
static void *buffers[8]; /* Actual cache allocations, retained in the ledger through free. */
static unsigned allocated; /* Number of real cache allocations, under ledger. */
static unsigned released; /* Number of actual frees of tracked buffers. */
static unsigned arrived; /* First allocation contenders, under ledger. */
static unsigned private_grows; /* Genuine replacement of an existing private cache. */
static unsigned writes; /* Actual runetochar destinations in already published cache. */
static int published; /* A caller has already received a view of this cache. */
static size_t capacity; /* Valid native extent for tracked buffers. */

void __real_Sys_werrstr(void *);
void *__real_malloc(size_t);
void __real_free(void *);
int __real_runetochar(char *, Rune *);
void __real_dvm_close_worker_stop(void);


/* EN: Observe the shared native input while forwarding the real frame unchanged.
 * RU: Наблюдать общий native-вход, передавая настоящий frame без изменений. */
void __wrap_Sys_werrstr(void *frame)
{
    F_Sys_werrstr call;
    memmove(&call, frame, sizeof(call));
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    pthread_mutex_lock(&ledger);
    if (!shared && process && process->pid > 1)
    {
        shared = call.s;
        if (shared->len >= 0)
        {
            fprintf(stderr, "FAIL expected genuine rune-backed shared String\n");
            exit(12);
        }
        capacity = (size_t)(-shared->len) * UTFmax + UTFmax;
    }
    observing = call.s == shared;
    expected_size = capacity;
    pthread_mutex_unlock(&ledger);
    uintptr_t old_cache = !observing && call.s && call.s != H ? (uintptr_t)call.s->tmp : 0;
    __real_Sys_werrstr(frame);
    if (!observing && old_cache && old_cache != (uintptr_t)call.s->tmp)
    {
        pthread_mutex_lock(&ledger);
        private_grows++;
        pthread_mutex_unlock(&ledger);
    }
    if (observing)
    {
        pthread_mutex_lock(&ledger);
        published = 1;
        pthread_mutex_unlock(&ledger);
    }
    observing = 0;
}


/* EN: Forward native allocation. Briefly align cache contenders, with a bounded
 * timeout so an ordinary cache mutex may correctly admit just one allocator.
 * RU: Выполнить нативное выделение. Ненадолго совместить запросы кэша; таймаут
 * позволяет обычному mutex корректно пропустить лишь одного выделяющего. */
void *__wrap_malloc(size_t bytes)
{
    int track = observing && bytes == expected_size;
    if (track)
    {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_nsec += 250000000;
        if (deadline.tv_nsec >= 1000000000)
        {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000;
        }
        pthread_mutex_lock(&ledger);
        arrived++;
        pthread_cond_broadcast(&changed);
        while (arrived < 8)
        {
            if (pthread_cond_timedwait(&changed, &ledger, &deadline))
            {
                break;
            }
        }
        pthread_mutex_unlock(&ledger);
    }
    void *result = __real_malloc(bytes);
    if (track && result)
    {
        pthread_mutex_lock(&ledger);
        if (allocated == 8)
        {
            fprintf(stderr, "FAIL shared cache allocated more than eight buffers\n");
            exit(12);
        }
        buffers[allocated++] = result;
        pthread_mutex_unlock(&ledger);
    }
    return result;
}


/* EN: Count a tracked native cache release before forwarding free exactly once.
 * RU: Учесть освобождение нативного кэша и передать free ровно один раз. */
void __wrap_free(void *value)
{
    pthread_mutex_lock(&ledger);
    for (unsigned at = 0; at < allocated; at++)
    {
        if (value && value == buffers[at])
        {
            released++;
            buffers[at] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&ledger);
    __real_free(value);
}


/* EN: Observe only real rune encoding into a cache already returned to readers.
 * Stack-local encoding is untracked; the actual primitive always executes.
 * RU: Наблюдать кодирование в уже возвращённый читателям кэш. Локальный буфер
 * стека не учитывается; настоящий примитив всегда выполняется. */
int __wrap_runetochar(char *output, Rune *rune)
{
    if (observing)
    {
        uintptr_t address = (uintptr_t)output;
        pthread_mutex_lock(&ledger);
        for (unsigned at = 0; published && at < allocated; at++)
        {
            uintptr_t base = (uintptr_t)buffers[at];
            if (base && address >= base && address - base < capacity)
            {
                writes++;
                break;
            }
        }
        pthread_mutex_unlock(&ledger);
    }
    return __real_runetochar(output, rune);
}


/* EN: After genuine GC/owner shutdown verify exact cache ownership and immutability.
 * RU: После настоящего завершения GC/владельцев проверить кэш и неизменяемость. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    fprintf(stderr, "UTF8_CACHE allocated=%u freed=%u published_writes=%u\n", allocated, released, writes);
    fprintf(stderr, "UTF8_PRIVATE cache_grows=%u\n", private_grows);
    if (allocated != 1 || released != 1 || writes || private_grows < 8)
    {
        fprintf(stderr, "FAIL shared UTF-8 cache ownership or immutable reads\n");
        exit(12);
    }
}
