/* Copyright (c) 2026 bhgv
 * EN: Real native closer cancellation, sticky replay, lifecycle failures and scope isolation.
 * RU: Настоящая отмена native closer, sticky replay, отказы lifecycle и изоляция scope.
 */
#include "close_worker.h"
#include "cancel.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protect deterministic native observations only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Explicit admission and retirement predicates. */
static int replay; /* Hold first callback before subscription to exercise sticky replay. */
static int admitted; /* Actual first consuming close reached native wait. */
static int requested; /* Shutdown cancellation completed before replay subscription. */
static int fail_source; /* Refuse one native source allocation. */
static int fail_thread; /* Refuse one service thread construction. */
static unsigned consumed; /* Exact FIFO ownership consumption count. */
static unsigned skipped; /* Already-cancelled jobs submitted behind the active close. */
static unsigned probes; /* Dynamic exclusive-close query count. */
DVM_Cancel *__real_dvm_cancel_create(void);
void __real_dvm_cancel_request(DVM_Cancel *source);
FVM_PortThread *__real_fvm_port_thread_start(FVM_PortThreadFn function, void *state);


/* EN/RU: Verify contracts in every optimization/sanitizer mode; terminate on refusal.
 * Проверить contracts во всех режимах optimization/sanitizer; завершить при отказе. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native close cancellation: %s\n", why);
        exit(2);
    }
}


/* EN/RU: One fixture handle owns this scalar state through its single consuming close.
 * Один fixture handle владеет scalar state до единственного consuming close. */
typedef struct Node
{
    unsigned id; /* Expected FIFO identity1..5. */
} Node;


/* EN/RU: Independently allocated callback state contains no handle/provider/guest pointer.
 * Независимый callback state не содержит handle/provider/guest pointer. */
typedef struct Wake
{
    pthread_mutex_t lock; /* Protect cancellation delivery. */
    pthread_cond_t changed; /* Native consuming-close wake. */
    int stopped; /* Idempotent sticky signal. */
} Wake;


/* EN/RU: Native nonwaiting signal only; source callback never enters a close/service lock.
 * Только native nonwaiting signal; callback не входит в close/service lock. */
static void interrupt(void *state)
{
    Wake *wake = state;
    pthread_mutex_lock(&wake->lock);
    wake->stopped = 1;
    pthread_cond_broadcast(&wake->changed);
    pthread_mutex_unlock(&wake->lock);
}


/* EN/RU: Consume native signal storage after every admitted callback has returned.
 * Поглотить native signal storage после завершения admitted callbacks. */
static void release(void *state)
{
    Wake *wake = state;
    pthread_cond_destroy(&wake->changed);
    pthread_mutex_destroy(&wake->lock);
    free(wake);
}


/* EN/RU: Consume native ownership in FIFO order; active first close blocks until shutdown signal.
 * Queued capable closes must observe pre-cancel; legacy close must receive no source.
 * Поглотить native ownership в FIFO; first close ждёт shutdown signal, queued
 * capable closes видят pre-cancel, legacy close не получает source. */
static void closefile(void *state)
{
    Node *node = state;
    DVM_Cancel *source = dvm_cancel_current();
    if (node->id == 1)
    {
        check(source != NULL, "active close has no cancellation scope");
        if (replay)
        {
            pthread_mutex_lock(&lock);
            admitted = 1;
            pthread_cond_broadcast(&changed);
            while (!requested)
            {
                pthread_cond_wait(&changed, &lock);
            }
            pthread_mutex_unlock(&lock);
        }
        Wake *wake = calloc(1, sizeof(*wake));
        check(wake && !pthread_mutex_init(&wake->lock, NULL) &&
              !pthread_cond_init(&wake->changed, NULL), "wake setup");
        DVM_CancelTap *tap = dvm_cancel_subscribe(source, interrupt, wake, release);
        check(tap != NULL, "close subscription");
        pthread_mutex_lock(&lock);
        admitted = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&lock);
        pthread_mutex_lock(&wake->lock);
        while (!wake->stopped)
        {
            pthread_cond_wait(&wake->changed, &wake->lock);
        }
        pthread_mutex_unlock(&wake->lock);
        dvm_cancel_unsubscribe(tap);
    }
    else if (node->id == 2 || node->id == 3)
    {
        check(source && dvm_cancel_requested(source), "queued close missed pre-cancel");
        skipped++;
    }
    else if (node->id == 4)
    {
        check(source == NULL, "legacy close inherited cancellation TLS");
    }
    else
    {
        check(source && !dvm_cancel_requested(source), "restarted service inherited old cancellation");
    }
    pthread_mutex_lock(&lock);
    check(node->id == consumed+1, "duplicate or reordered consuming close");
    consumed++;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    free(node);
}
static const DVM_VfsOps operations = {NULL, NULL, NULL, NULL, NULL, closefile}; /* Pure native consuming fixture. */


/* EN/RU: Inspect exclusive scalar state before scope entry; no allocation, I/O or retention.
 * Проверить exclusive scalar state до входа в scope; без allocation/I/O/retention. */
static int ready(void *state)
{
    check(((Node *)state)->id == 3 && !dvm_cancel_current(), "dynamic close query ownership/context");
    probes++;
    return 1;
}


/* EN/RU: Preserve normal allocation except one deterministic service-start refusal.
 * Сохранить normal allocation кроме одного детерминированного отказа service-start. */
DVM_Cancel *__wrap_dvm_cancel_create(void)
{
    if (fail_source)
    {
        fail_source = 0;
        return NULL;
    }
    return __real_dvm_cancel_create();
}


/* EN/RU: Preserve source request, then permit the first callback's deliberately late subscription.
 * Сохранить source request, затем разрешить намеренно позднюю subscription first callback. */
void __wrap_dvm_cancel_request(DVM_Cancel *source)
{
    __real_dvm_cancel_request(source);
    pthread_mutex_lock(&lock);
    requested = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Refuse thread construction before adopting function/state; otherwise run the real port.
 * Отказать thread construction до принятия function/state; иначе выполнить настоящий port. */
FVM_PortThread *__wrap_fvm_port_thread_start(FVM_PortThreadFn function, void *state)
{
    if (fail_thread)
    {
        fail_thread = 0;
        return NULL;
    }
    return __real_fvm_port_thread_start(function, state);
}


/* EN/RU: Allocate job and exclusive handle before queue publication; verify linear transfer.
 * Выделить job и exclusive handle до queue publication; проверить linear transfer. */
static void submit(unsigned id)
{
    DVM_CloseJob *job = dvm_close_prepare();
    Node *node = calloc(1, sizeof(*node));
    check(job && node, "preallocated job");
    node->id = id;
    DVM_VfsHandle handle = {0};
    handle.ops = &operations;
    handle.state = node;
    handle.native_flags = id == 4 || id == 3 ? 0 : DVM_VFS_CANCEL_CLOSE;
    handle.close_ready = id == 3 ? ready : NULL;
    dvm_close_submit_resource(job, &handle);
    check(!handle.ops && !handle.state && !handle.close_ready, "submission did not clear owning handle");
}


/* EN/RU: Exercise two start failures, active/pre-subscribe cancellation, queued/legacy isolation and restart.
 * Проверить два start failure, active/pre-subscribe cancellation, queued/legacy isolation и restart. */
int main(int argc, char **argv)
{
    check(argc == 2, "mode argument");
    replay = !strcmp(argv[1], "replay");
    fail_source = 1;
    check(!dvm_close_worker_start(), "source refusal leaked a running service");
    fail_thread = 1;
    check(!dvm_close_worker_start(), "thread refusal leaked a running service");
    check(dvm_close_worker_start(), "service start after failures");
    submit(1);
    pthread_mutex_lock(&lock);
    while (!admitted)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
    submit(2);
    submit(3);
    submit(4);
    dvm_close_worker_stop();
    check(consumed == 4 && skipped == 2 && probes == 1 && !dvm_cancel_current(), "native drain/scope isolation");
    dvm_close_statistics();
    check(dvm_close_worker_start(), "service restart");
    submit(5);
    pthread_mutex_lock(&lock);
    while (consumed != 5)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
    dvm_close_worker_stop();
    dvm_close_worker_stop();
    puts("PASS native close cancellation replay ownership and restart");
    return 0;
}
