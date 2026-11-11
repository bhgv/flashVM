/* Copyright (c) 2026 bhgv
 * EN: Audit the channel reservation snapshot's native mutex on real GC threads.
 * RU: Проверка нативного мьютекса снимка резерваций на настоящих GC-потоках.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>

static _Thread_local int querying; /* Only this native thread's snapshot scope. */
static _Thread_local unsigned entries; /* Acquisitions in the current snapshot. */
static _Thread_local unsigned held; /* Balanced held state in the current query. */
static _Thread_local FVM_PortSync *guard; /* Borrowed snapshot mutex identity. */
static unsigned long queries; /* Atomic completed real GC snapshots. */

int __real_dvm_runtime_channel_buffer_busy_snapshot(Channel *);
void __real_fvm_port_sync_lock(FVM_PortSync *);
void __real_fvm_port_sync_unlock(FVM_PortSync *);


/* EN: Fail the private audit with a native diagnostic, without accessing VM data.
 * RU: Завершить частную проверку нативной диагностикой без доступа к данным VM. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL channel GC snapshot: %s\n", message);
        exit(10);
    }
}


/* EN: Forward the real lock; record only acquisitions inside this query scope.
 * sync remains owned by the runtime. Other VM locks are forwarded untouched.
 * RU: Вызвать настоящий lock и учесть захват только внутри текущего снимка.
 * sync принадлежит runtime. Остальные блокировки передаются без изменений. */
void __wrap_fvm_port_sync_lock(FVM_PortSync *sync)
{
    __real_fvm_port_sync_lock(sync);
    if (querying)
    {
        require(!held, "snapshot does not nest mutex acquisitions");
        guard = sync;
        held = 1;
        entries++;
    }
}


/* EN: Balance the exact borrowed mutex, then forward its native unlock.
 * RU: Проверить снятие точного заимствованного мьютекса и вызвать unlock. */
void __wrap_fvm_port_sync_unlock(FVM_PortSync *sync)
{
    if (querying)
    {
        require(held && guard == sync, "snapshot unlock matches its acquisition");
        held = 0;
    }
    __real_fvm_port_sync_unlock(sync);
}


/* EN: Audit a real borrowed channel query; never replace its value or traversal.
 * TLS state distinguishes parallel GC queries; atomics count completed calls.
 * RU: Проверить настоящий запрос channel, не подменяя результат или обход.
 * TLS разделяет параллельные запросы GC, атомарный счётчик считает завершения. */
int __wrap_dvm_runtime_channel_buffer_busy_snapshot(Channel *channel)
{
    require(!querying, "snapshot scope is not recursive");
    querying = 1;
    entries = 0;
    held = 0;
    guard = NULL;
    int result = __real_dvm_runtime_channel_buffer_busy_snapshot(channel);
    require(entries == 1 && !held, "snapshot traverses under its native mutex");
    querying = 0;
    __atomic_fetch_add(&queries, 1, __ATOMIC_RELAXED);
    return result;
}


/* EN: Require actual snapshot coverage after all runtime and GC threads joined.
 * RU: Подтвердить реальные вызовы снимка после join потоков runtime и GC. */
__attribute__((destructor)) static void report(void)
{
    unsigned long count = __atomic_load_n(&queries, __ATOMIC_RELAXED);
    require(count > 0, "real collector snapshot path executed");
    fprintf(stderr, "PASS channel GC snapshot queries=%lu locked=1\n", count);
}
