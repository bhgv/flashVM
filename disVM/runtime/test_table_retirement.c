/* Copyright (c) 2026 bhgv
 * EN: Keep two real residents admitted while shared Fast reclaims native owners.
 * RU: Два настоящих потока остаются допущенными, пока Fast освобождает владельцев.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "disvm/native_owner.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Protect relay state, never VM payload. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Relay and final release notifications. */
static unsigned admitted; /* Peers have published shared roots, under sync. */
static unsigned turn; /* Only this peer may briefly advance its admission epoch. */
static int released; /* Parent completed the live-owner check. */
static int expired; /* A finite peer watchdog observed missing parent progress. */

int __real_dvm_runtime_guest_sleep(unsigned long);
void __real_Sys_werrstr(void *);
void dvm_bigtable_owners(unsigned long *, unsigned long *);


/* EN: Report a precise native fixture failure; message is borrowed constant text.
 * RU: Сообщить точную нативную ошибку теста; message — заимствованный текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL shared table retirement: %s\n", message);
        exit(12);
    }
}


/* EN: Return monotonic milliseconds for a bounded diagnostic, never a success oracle.
 * RU: Вернуть монотонное время для ограничения диагностики, не критерия успеха. */
static unsigned long long milliseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "monotonic clock available");
    return (unsigned long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}


/* EN: Hold the real sleep(0) before its native release, without touching registers.
 * Two peers alternate epoch advancement while the other stays admitted/shared;
 * quiescent-only fallback GC therefore cannot explain successful retirement.
 * Forward the original scalar duration unchanged after the parent releases both.
 * RU: Задержать настоящий sleep(0) до native release, не меняя регистры.
 * Два соседа по очереди обновляют эпоху, пока другой остаётся допущенным;
 * сборка только при общем покое не может объяснить успешное освобождение.
 * После разрешения родителя передать исходную длительность без изменений. */
int __wrap_dvm_runtime_guest_sleep(unsigned long duration)
{
    require(duration == 0, "fixture uses only the genuine zero-duration sleep");
    require(dvm_runtime_gc_dispatch_share(), "peer enters shared admission with real roots");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    unsigned long long deadline = milliseconds() + 15000;
    pthread_mutex_lock(&sync);
    unsigned id = admitted++;
    require(id < 2, "exactly two admitted peers");
    if (admitted == 2)
    {
        fprintf(stderr, "TABLE_RETIRE_PEERS_ADMITTED\n");
        pthread_cond_broadcast(&changed);
    }
    while (!released && milliseconds() < deadline)
    {
        if (admitted == 2 && turn == id)
        {
            /* The other peer is still shared and cannot leave while sync is held. */
            require(dvm_native_owner_leave_shared(owner) && dvm_native_owner_enter_shared(owner),
                "advance one grace period without heap quiescence");
            turn = 1 - id;
            pthread_cond_broadcast(&changed);
        }
        pthread_mutex_unlock(&sync);
        fvm_port_thread_sleep_us(1000);
        pthread_mutex_lock(&sync);
    }
    if (!released)
    {
        expired = 1;
        released = 1;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_dispatch_finish();
    return __real_dvm_runtime_guest_sleep(duration);
}


/* EN: Run real werrstr, then require only the two anchors to remain before release.
 * frame remains production-owned/rooted. No heap object, collector or payload is
 * mocked; counters are sampled outside admission while both peers stay shared.
 * RU: Выполнить настоящий werrstr и оставить только два anchor до снятия барьера.
 * frame принадлежит runtime и укоренён. Куча, сборщики и payload не подменяются;
 * счётчики читаются вне допуска, пока оба соседа остаются в shared. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent publishes native roots while sampling live owners");
    unsigned long tables = 0;
    unsigned long keys = 0;
    unsigned long long deadline = milliseconds() + 5000;
    do
    {
        dvm_bigtable_owners(&tables, &keys);
        if (tables == 2 && keys == 2)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    } while (milliseconds() < deadline);
    pthread_mutex_lock(&sync);
    int success = tables == 2 && keys == 2 && admitted == 2 && !expired;
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_native_enter(native);
    require(success, "dead native table owners require heap quiescence");
    fprintf(stderr, "PASS shared native table retirement tables=2 keys=2 admitted=2\n");
}
