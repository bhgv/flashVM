/* Copyright (c) 2026 bhgv
 * [GC:] EN: Force a collector quietness observation against an occupied channel queue.
 * RU: Принудительно проверить GC-наблюдение канала при занятой очереди.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "gc/common/graph_snapshot.h"
#include "../../flashVM/ports/channel_sync.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


extern void __real_dvm_proxyhost_initialize(void);
extern int dvm_runtime_channel_is_quiet(Channel *channel);
extern void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument);


/* [GC:] EN: Private probe owns its synthetic empty channel and thread until join.
 * gate protects done/quiet; queue owner never reads worker state without gate.
 * RU: Проба владеет пустым каналом и потоком до join. gate защищает done/quiet;
 * владелец очереди не читает состояние работника без gate. */
typedef struct QueueProbe
{
    Heap heap;              /* Synthetic header, only the channel/BUSY path may inspect it. */
    Channel channel;        /* Private empty channel, never published to guests. */
    pthread_mutex_t gate;   /* Protects completion and result. */
    pthread_cond_t changed; /* Completion notification, default realtime clock. */
    pthread_t worker;       /* Joined after releasing the runtime queue mutex. */
    int done;               /* Worker returned from the real quietness query. */
    int capture_busy;       /* Real graph discovery preserved transient contention. */
    int quiet;              /* Query result; contention must return minus one. */
    int failed;             /* Timed wait or thread creation failed. */
} QueueProbe;


/* [GC:] EN: Query probe's live channel while parent owns the queue mutex.
 * Exercise real graph discovery on synthetic channel metadata through BUSY only.
 * Publish both results under gate; no managed root or guest identity is fabricated.
 * RU: Проверить живой канал probe, пока родитель владеет мьютексом очереди.
 * Проверить настоящий BUSY-путь discovery на синтетических metadata канала.
 * Опубликовать результаты под gate; managed root и guest identity не подменяются. */
static void *observe_queue(void *argument)
{
    QueueProbe *probe = argument;
    int quiet = dvm_runtime_channel_is_quiet(&probe->channel);
    int capture_busy = 0;
    if (quiet == -1)
    {
        RecordGroupScratch scratch = {0};
        RecordGroup group = dvm_rc_empty_record_group(&scratch);
        int scanned = dvm_rc_queued_record_group(&probe->channel, 1, &group, NULL);
        capture_busy = !scanned && group.busy;
    }
    pthread_mutex_lock(&probe->gate);
    probe->quiet = quiet;
    probe->capture_busy = capture_busy;
    probe->done = 1;
    pthread_cond_signal(&probe->changed);
    pthread_mutex_unlock(&probe->gate);
    return NULL;
}


/* [GC:] EN: Called under the actual queue mutex. Launch a real observer and
 * wait at most five seconds for nonblocking refusal. Return before joining:
 * even the broken blocking implementation can then unwind, never hang the suite.
 * probe owns all synchronization; failed preserves every unsuccessful wait.
 * RU: Под настоящим мьютексом очереди запустить наблюдателя и ждать не более
 * пяти секунд отказа без блокировки. Вернуться до join, чтобы ошибочный вариант
 * тоже завершился после unlock. probe владеет синхронизацией и хранит отказы. */
static int occupy_queue(void *argument)
{
    QueueProbe *probe = argument;
    struct timespec deadline;
    if (clock_gettime(CLOCK_REALTIME, &deadline) || pthread_create(&probe->worker, NULL, observe_queue, probe))
    {
        fprintf(stderr, "queue probe could not start\n");
        exit(87);
    }
    deadline.tv_sec += 5;
    pthread_mutex_lock(&probe->gate);
    while (!probe->done && !probe->failed)
    {
        probe->failed = pthread_cond_timedwait(&probe->changed, &probe->gate, &deadline) != 0;
    }
    pthread_mutex_unlock(&probe->gate);
    return 1;
}


/* [GC:] EN: Initialize the real host, then force queue contention before guest
 * startup. Borrow no heap roots; join observer and destroy probe before return.
 * Exit 86 for a blocking observation, 87 for fixture setup failure.
 * RU: Инициализировать host и создать конфликт очереди до старта гостей.
 * Корни heap не заимствуются; join и уничтожение probe предшествуют возврату.
 * Код 86 означает блокирующее наблюдение, 87 — ошибку подготовки теста. */
void __wrap_dvm_proxyhost_initialize(void)
{
    QueueProbe probe = {0};
    __real_dvm_proxyhost_initialize();
    if (D2H(&probe.channel) != &probe.heap)
    {
        exit(87);
    }
    probe.heap.t = &Tchannel;
    if (pthread_mutex_init(&probe.gate, NULL) || pthread_cond_init(&probe.changed, NULL))
    {
        exit(87);
    }
    dvm_runtime_channel_queue_guard(occupy_queue, &probe);
    if (pthread_join(probe.worker, NULL))
    {
        exit(87);
    }
    pthread_cond_destroy(&probe.changed);
    pthread_mutex_destroy(&probe.gate);
    if (probe.failed || !probe.done || !probe.capture_busy || probe.quiet != -1 ||
        dvm_runtime_channel_is_quiet(&probe.channel) != 1)
    {
        fprintf(stderr, "FAIL GC queue observation blocked or lost retry\n");
        exit(86);
    }
    fprintf(stderr, "PASS GC quiet observation refuses occupied queue\n");
}


#if CONTROL == 1
extern void fvm_port_sync_lock(FVM_PortSync *sync);


/* [GC:] EN: Private negative control restores the old blocking observation.
 * sync remains borrowed; return owning its mutex exactly like a successful try.
 * RU: Частный отрицательный контроль возвращает прежнее ожидание. sync
 * заимствован; вернуть владение мьютексом как при успешной попытке. */
int __wrap_fvm_port_sync_try_lock(FVM_PortSync *sync)
{
    fvm_port_sync_lock(sync);
    return 1;
}
#endif


#if CONTROL == 2
extern int __real_dvm_rc_queued_record_group(void *, unsigned long, RecordGroup *, DVM_RangeAccess *);


/* [GC:] EN: Omit only transient retry after real graph discovery. All borrowed
 * inputs and returned scan status survive; the acceptance must reject lost busy.
 * RU: После настоящего discovery убрать только transient retry. Заимствованные
 * параметры и scan status сохранены; тест обязан обнаружить потерю busy. */
int __wrap_dvm_rc_queued_record_group(void *start, unsigned long roots, RecordGroup *group, DVM_RangeAccess *access)
{
    int scanned = __real_dvm_rc_queued_record_group(start, roots, group, access);
    group->busy = 0;
    return scanned;
}
#endif
