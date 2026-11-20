/* Copyright (c) 2026 bhgv
 * EN: Detached native close FIFO with mutex-protected publication and snapshots.
 * RU: Нативная FIFO закрытий с публикацией и снимками под мьютексом.
 */
/* Native close queue: finalizers detach ownership without waiting on filesystem I/O. */
#include "close_worker.h"
#include "channel_sync.h"
#include "thread_port.h"
#include "vfs/cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Preallocated before open; exclusively transferred once after gate retirement.
 * Выделен до open; эксклюзивно передаётся один раз после gate retirement. */
struct DVM_CloseJob
{
    DVM_CloseJob *next;          /* Native queue link, protected by service.sync. */
    DVM_VfsHandle resource; /* Exclusive provider handle; no managed owner survives. */
};


/* EN/RU: Service owns detached native handles and cancellation TLS, never guest TLS/GC/REG.
 * Служба владеет detached native handles/cancellation TLS, но не guest TLS/GC/REG. */
typedef struct CloseService
{
    FVM_PortSync *sync;          /* Protect queue/counters/stop, never held through close. */
    DVM_Cancel *cancel; /* Owned shutdown source; native close scopes hold independent pins. */
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

static __thread int draining; /* The single closer may enqueue dependent retirement jobs. */
static CloseService service; /* One runtime lifetime; cleared only at explicit start. */


/* EN/RU: Consume exclusive job/resource and free its reservation, outside the queue mutex.
 * Query current native close capability; enter/leave only a native source scope when eligible.
 * No managed access or guest callbacks; legacy close may block. No result.
 * Поглотить exclusive job/resource и reservation вне queue mutex; capability query
 * выбирает native source scope. Без VM/guest callbacks; legacy close может ждать. */
static void dispose(DVM_CloseJob *job)
{
    DVM_CancelScope scope = {0};
    int scoped = service.cancel && dvm_vfs_close_cancellable(&job->resource);
    if (scoped && !dvm_cancel_enter(&scope, service.cancel))
    {
        fprintf(stderr, "disvm: native close cancellation scope refused\n");
        abort();
    }
    dvm_vfs_close(&job->resource);
    if (scoped && !dvm_cancel_leave(&scope))
    {
        fprintf(stderr, "disvm: native close cancellation scope ownership lost\n");
        abort();
    }
    free(job);
}


/* EN/RU: Native thread callback; unused is NULL. FIFO and wake predicate share a mutex.
 * Only native cancellation TLS is used; no managed heap/REG admission.
 * Native thread callback; unused NULL; FIFO/wake predicate под одним mutex,
 * только native cancellation TLS, без managed heap/REG admission. */
static void execute(void *unused)
{
    (void)unused;
    draining = 1;
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
    draining = 0;
}


/* EN/RU: See header; source/thread setup failure releases all partial state before producers start.
 * См. header; source/thread setup failure освобождает partial state до producers. */
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
    service.cancel = dvm_cancel_create();
    if (!service.cancel)
    {
        fvm_port_sync_destroy(service.sync);
        service.sync = NULL;
        return 0;
    }
    service.thread = fvm_port_thread_start(execute, NULL);
    if (!service.thread)
    {
        dvm_cancel_release(service.cancel);
        service.cancel = NULL;
        fvm_port_sync_destroy(service.sync);
        service.sync = NULL;
        return 0;
    }
    return 1;
}


/* EN/RU: See header; allocate a zeroed exclusive retirement job before resource creation.
 * См. header; выделить zeroed exclusive retirement job до создания resource. */
DVM_CloseJob *dvm_close_prepare(void)
{
    return calloc(1, sizeof(DVM_CloseJob));
}


/* EN/RU: See header; consume only unused reservations, never queued ownership.
 * См. header; поглотить только unused reservations, не queued ownership. */
void dvm_close_discard(DVM_CloseJob *job)
{
    free(job);
}


/* EN/RU: See header; adapt exclusive file/directory before transferring a preallocated job.
 * Caller owns table/GC exclusion; publication performs no I/O or managed reentry.
 * См. header; адаптировать exclusive file/directory до передачи job; caller владеет
 * table/GC exclusion, publication без I/O и managed reentry. */
void dvm_close_submit(DVM_CloseJob *job, FVM_PortFile *file, FVM_PortDirectory *directory)
{
    DVM_VfsHandle resource = {0};
    if (!dvm_vfs_adopt(&resource, file, directory))
    {
        abort();
    }
    dvm_close_submit_resource(job, &resource);
}


/* EN/RU: See close_worker.h; move before queue publication, never under I/O lock.
 * См. close_worker.h; перенос до публикации в очередь, без I/O под lock. */
void dvm_close_submit_resource(DVM_CloseJob *job, DVM_VfsHandle *resource)
{
    if (!job)
    {
        fprintf(stderr, "disvm: missing native close reservation\n");
        exit(1);
    }
    if (!dvm_vfs_move(&job->resource, resource))
    {
        abort();
    }
    job->next = NULL;
    if (!service.sync)
    {
        dispose(job);
        return;
    }
    fvm_port_sync_lock(service.sync);
    if (service.stopping && !draining)
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


/* EN/RU: See header; all producers must be joined. Signal outside the queue mutex,
 * join without heap admission; the closer may enqueue dependent work while draining.
 * Release the service source and sync after join. Idempotent.
 * См. header; producers уже joined. Signal вне mutex, join без heap admission,
 * closer может поставить зависимые закрытия в очередь до выхода. После join
 * освободить source/sync. Повторный вызов безопасен. */
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
    dvm_cancel_request(service.cancel);
    fvm_port_thread_join(service.thread);
    service.thread = NULL;
    dvm_cancel_release(service.cancel);
    service.cancel = NULL;
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
