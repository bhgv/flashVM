/* Copyright (c) 2026 bhgv
 * EN: Real concurrent preparation cancellation, outer-source forwarding and callback retirement.
 * RU: Настоящая concurrent отмена подготовки, outer-source forwarding и callback retirement.
 */
#include "backend.h"
#include "../../../runtime/vfs/launch/ticket.h"
#include <pthread.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Protects native admission/retirement counters. */
static pthread_cond_t admitted = PTHREAD_COND_INITIALIZER; /* Actual subscription admission signal. */
static unsigned started; /* Actual held root callbacks. */
static unsigned retired; /* Exactly retired independent callback contexts. */
static DVM_Cancel *outer_source; /* Borrowed outer signal must never be reused as ticket scope. */
static LaunchState *blocked; /* Fixture-selected provider state, never retained by cancellation callback. */


/* EN/RU: Independent native cancellation state; callback has no root/request/guest pointer.
 * Independent native state без root/request/guest pointers. */
typedef struct Signal
{
    pthread_mutex_t lock; /* Protects stopped flag. */
    pthread_cond_t wake; /* Native wait interrupt, no guest continuation. */
    int stopped; /* Sticky signal predicate. */
} Signal;


/* EN/RU: Signal only native readiness outside all provider/shared-state mutexes.
 * Сигнализировать native readiness вне provider/shared-state mutexes. */
static void interrupt(void *opaque)
{
    Signal *signal = opaque;
    pthread_mutex_lock(&signal->lock);
    signal->stopped = 1;
    pthread_cond_broadcast(&signal->wake);
    pthread_mutex_unlock(&signal->lock);
}


/* EN/RU: Consume native state only after every admitted signal completes.
 * Поглотить native state после завершения всех admitted signals. */
static void signal_free(void *opaque)
{
    Signal *signal = opaque;
    pthread_cond_destroy(&signal->wake);
    pthread_mutex_destroy(&signal->lock);
    free(signal);
    pthread_mutex_lock(&gate);
    retired++;
    pthread_mutex_unlock(&gate);
}


/* EN/RU: Block actual metadata until its current native cancellation source fires.
 * Callback state is independent, without provider pointers; acknowledge metadata after interruption.
 * Ждать current native source в metadata; callback state independent, затем подтвердить metadata. */
static FVM_Result held_info(void *state, const char *path, FVM_FileInfo *out)
{
    if (state == blocked && !strcmp(path, "."))
    {
        launch_check(dvm_cancel_current() && dvm_cancel_current() != outer_source, "missing independent preparation scope");
        Signal *signal = calloc(1, sizeof(*signal));
        launch_check(signal && !pthread_mutex_init(&signal->lock, NULL) &&
            !pthread_cond_init(&signal->wake, NULL), "signal allocation");
        DVM_CancelTap *tap = dvm_cancel_subscribe(dvm_cancel_current(), interrupt, signal, signal_free);
        launch_check(tap != NULL, "missing preparation cancellation scope");
        pthread_mutex_lock(&gate);
        started++;
        pthread_cond_broadcast(&admitted);
        pthread_mutex_unlock(&gate);
        pthread_mutex_lock(&signal->lock);
        while (!signal->stopped)
        {
            pthread_cond_wait(&signal->wake, &signal->lock);
        }
        pthread_mutex_unlock(&signal->lock);
        dvm_cancel_unsubscribe(tap);
    }
    return launch_info(state, path, out);
}
static const DVM_ProviderOps held_ops = {held_info, launch_open, launch_destroy, DVM_PROVIDER_STABLE_BIND}; /* Real native wait at either root. */


/* EN/RU: Worker borrows stable request/provider/origin and owns one outer cancellation scope.
 * Worker заимствует request/provider/origin и владеет outer scope до join. */
typedef struct Worker
{
    DVM_LaunchRequest *request; /* Borrowed through join. */
    DVM_Provider *provider; /* Borrowed granted provider through join. */
    DVM_Origin origin; /* Copied provider association. */
    DVM_Cancel *parent; /* Borrowed main-owned outer source. */
    int result; /* Preparation result, inspected after join. */
} Worker;


/* EN/RU: Prepare under an outer source and require exact TLS restoration before leaving it.
 * Подготовить под outer source; проверить восстановление TLS до выхода из outer scope. */
static void *prepare(void *opaque)
{
    Worker *worker = opaque;
    DVM_CancelScope outer = {0};
    launch_check(dvm_cancel_enter(&outer, worker->parent), "outer source entry");
    worker->result = dvm_launch_request_prepare(worker->request, worker->provider, &worker->origin);
    launch_check(dvm_cancel_current() == worker->parent && dvm_cancel_leave(&outer) &&
        !dvm_cancel_current(), "outer source identity/lifetime lost");
    return NULL;
}


/* EN/RU: Cancel/refuse after real native admission, or start with a pre-cancelled outer source.
 * Kind selects requester cancel0, outer1, provider refusal2, pre-cancel3, final-root4 or duplicate5.
 * Cancel/refuse после native admission либо pre-cancel; kind выбирает проверяемую границу. */
static void scenario(int kind)
{
    LaunchState terminal = {0}, provider = {0};
    terminal.terminal = 1;
    DVM_LaunchOptions options = {0};
    options.policy = DVM_LAUNCH_TERMINAL_FIRST;
    options.terminal = dvm_provider_create_native(&held_ops, &terminal, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    options.provider = dvm_provider_create_native(&held_ops, &provider, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    launch_origin(&options.terminal_origin, DVM_ORIGIN_TERMINAL, 'T');
    launch_origin(&options.provider_origin, DVM_ORIGIN_PROVIDER, 'P');
    DVM_LaunchSpec input = {0};
    input.policy = options.policy;
    input.terminal = options.terminal;
    input.terminal_origin = options.terminal_origin;
    strcpy(input.expected_provider, options.provider_origin.system);
    strcpy(input.module, "remote-entry");
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&input, &request);
    launch_check(ticket && request, "wait request construction");
    blocked = kind == 4 ? &provider : &terminal;
    DVM_Cancel *parent = dvm_cancel_create();
    launch_check(parent != NULL, "outer source construction");
    if (kind == 3)
    {
        dvm_cancel_request(parent);
    }
    outer_source = parent;
    Worker worker = {request, options.provider, options.provider_origin, parent, -1};
    pthread_t thread;
    unsigned before = started;
    launch_check(!pthread_create(&thread, NULL, prepare, &worker), "prepare thread");
    if (kind != 3)
    {
        pthread_mutex_lock(&gate);
        while (started == before)
        {
            pthread_cond_wait(&admitted, &gate);
        }
        pthread_mutex_unlock(&gate);
        launch_check(dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PREPARING, "admitted state");
        if (kind == 1)
        {
            dvm_cancel_request(parent);
        }
        else if (kind == 2)
        {
            launch_check(dvm_launch_request_refuse(request), "concurrent provider refusal");
        }
        else
        {
            if (kind == 5)
            {
                launch_check(!dvm_launch_request_prepare(request, options.provider, &options.provider_origin),
                             "duplicate preparation admitted");
            }
            launch_check(dvm_launch_ticket_cancel(ticket), "concurrent requester cancellation");
        }
    }
    launch_check(!pthread_join(thread, NULL), "prepare join");
    launch_check(!worker.result && !dvm_launch_request_take(request) &&
        dvm_launch_ticket_status(ticket) == (kind == 2 ? DVM_LAUNCH_DENIED : DVM_LAUNCH_CANCELLED),
        "cancelled environment became available");
    launch_check(kind == 4 ? terminal.roots == 1 && provider.roots == 1 : provider.roots == 0,
                 "preparation continued into later root after cancellation");
    launch_check(dvm_cancel_requested(parent) == (kind == 1 || kind == 3), "ticket cancelled unrelated outer source");
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_cancel_release(parent);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "cancelled root ownership leaked");
}


/* EN/RU: Run native cancellation races without sleeps or guest polling, then require exact callback drain.
 * Проверить native races без sleeps/guest polling и exact callback drain. */
int main(void)
{
    for (int kind = 0; kind < 6; kind++)
    {
        scenario(kind);
    }
    launch_check(started == 5 && retired == started, "independent callback state leaked");
    puts("PASS launch native cancel refusal outer forwarding one-attempt and callback drain");
    return 0;
}
