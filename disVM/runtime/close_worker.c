/* Copyright (c) 2026 bhgv
 * EN: Detached native close FIFO with mutex-protected publication and snapshots.
 * RU: Нативная FIFO закрытий с публикацией и снимками под мьютексом.
 */
/* Native close queue: finalizers detach ownership without waiting on filesystem I/O. */
#include "close_worker.h"
#include "channel_sync.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Prepared before file creation, transferred exactly once after gate retirement. */
struct DVM_CloseJob
{
    DVM_CloseJob *next;          /* Native queue link, protected by service.sync. */
    FVM_PortFile *file;          /* Exclusive native handle; no managed owner survives. */
    FVM_PortDirectory *directory; /* Exclusive native cursor, or NULL. */
};


/* The worker owns only detached handles. No TLS/GC/REG or managed callbacks. */
typedef struct CloseService
{
    FVM_PortSync *sync;          /* Protect queue/counters/stop, never held through close. */
    FVM_PortThread *thread;      /* Joined after producers stop, before sync destruction. */
    DVM_CloseJob *head;          /* Pending FIFO; native allocations prepared before open. */
    DVM_CloseJob *tail;          /* Last queued job, NULL exactly when head is NULL. */
    unsigned long submitted;    /* Successful ownership transfers, under sync. */
    unsigned long completed;    /* Jobs closed/freed, under sync. */
    unsigned long pending;      /* Queued plus currently closing jobs. */
    unsigned long peak;         /* Largest simultaneous detached handle backlog. */
    int stopping;               /* Set after producer shutdown; drain before exit. */
    int joined;                 /* One after a successfully started worker joins. */
} CloseService;

static CloseService service; /* One runtime lifetime; cleared only at explicit start. */


/* Consume detached native ownership. job is exclusive; handles may be NULL.
 * Port close can block but cannot access managed memory or execute VM callbacks. */
static void dispose(DVM_CloseJob *job)
{
    fvm_port_file_close(job->file);
    fvm_port_directory_close(job->directory);
    free(job);
}


/* Native thread callback; unused must be NULL. FIFO publication uses the same
 * mutex as the predicate wait, preventing lost wakeups. No heap/TLS admission. */
static void execute(void *unused)
{
    (void)unused;
    fvm_port_sync_lock(service.sync);
    for (;;)
    {
        DVM_CloseJob *job;
        while (!service.head && !service.stopping)
        {
            fvm_port_sync_wait(service.sync);
        }
        if (!service.head)
        {
            break;
        }
        job = service.head;
        service.head = job->next;
        if (!service.head)
        {
            service.tail = NULL;
        }
        fvm_port_sync_unlock(service.sync);
        dispose(job);
        fvm_port_sync_lock(service.sync);
        service.completed++;
        service.pending--;
    }
    fvm_port_sync_unlock(service.sync);
}


/* See header: failure starts no producer-visible service and leaks no sync. */
int dvm_close_worker_start(void)
{
    if (service.sync)
    {
        return 0;
    }
    memset(&service, 0, sizeof(service));
    service.sync = fvm_port_sync_create();
    if (!service.sync)
    {
        return 0;
    }
    service.thread = fvm_port_thread_start(execute, NULL);
    if (!service.thread)
    {
        fvm_port_sync_destroy(service.sync);
        service.sync = NULL;
        return 0;
    }
    return 1;
}


/* The preallocated job prevents allocation failure in a GC finalizer. */
DVM_CloseJob *dvm_close_prepare(void)
{
    return calloc(1, sizeof(DVM_CloseJob));
}


/* Only unused native reservations are discarded, never a queued node. */
void dvm_close_discard(DVM_CloseJob *job)
{
    free(job);
}


/* Producers own table/GC exclusion, while native queue state uses sync. Finalizer
 * publication never waits for an in-progress close or reenters managed execution. */
void dvm_close_submit(DVM_CloseJob *job, FVM_PortFile *file, FVM_PortDirectory *directory)
{
    if (!job)
    {
        fprintf(stderr, "disvm: missing native close reservation\n");
        exit(1);
    }
    job->file = file;
    job->directory = directory;
    job->next = NULL;
    if (!service.sync)
    {
        dispose(job);
        return;
    }
    fvm_port_sync_lock(service.sync);
    if (service.stopping)
    {
        fvm_port_sync_unlock(service.sync);
        fprintf(stderr, "disvm: native close producer survived shutdown\n");
        exit(1);
    }
    if (service.tail)
    {
        service.tail->next = job;
    }
    else
    {
        service.head = job;
    }
    service.tail = job;
    service.submitted++;
    service.pending++;
    if (service.pending > service.peak)
    {
        service.peak = service.pending;
    }
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
}


/* GC and mutator producers must already be joined. The closer never needs heap
 * admission to finish, so this final join cannot form a heap/close lock cycle. */
void dvm_close_worker_stop(void)
{
    if (!service.sync)
    {
        return;
    }
    fvm_port_sync_lock(service.sync);
    service.stopping = 1;
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
    fvm_port_thread_join(service.thread);
    service.thread = NULL;
    service.joined = 1;
    fvm_port_sync_destroy(service.sync);
    service.sync = NULL;
}


/* EN: Snapshot native close totals; sync lifetime is stable throughout the call.
 * Normally called after stop; the explicit halt test can inspect live producers.
 * No arguments/result. Never hold the queue mutex through diagnostic I/O.
 * RU: Снять итоги закрытий; время жизни sync стабильно до возврата.
 * Обычно после stop; явный тест halt может видеть живых производителей.
 * Без параметров/результата. Не держать мьютекс очереди во время вывода. */
void dvm_close_statistics(void)
{
    if (service.sync)
    {
        fvm_port_sync_lock(service.sync);
    }
    unsigned long submitted = service.submitted;
    unsigned long completed = service.completed;
    unsigned long pending = service.pending;
    unsigned long peak = service.peak;
    int joined = service.joined;
    if (service.sync)
    {
        fvm_port_sync_unlock(service.sync);
    }
    fprintf(stderr, "NATIVECLOSE submitted=%lu completed=%lu pending=%lu peak=%lu joined=%d\n",
        submitted, completed, pending, peak, joined);
}
