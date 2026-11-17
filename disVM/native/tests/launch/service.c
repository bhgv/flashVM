/* Copyright (c) 2026 bhgv
 * EN: Bounded service admission, FIFO review, close, native readiness and concurrent capacity races.
 * RU: Ограниченный service admission, FIFO review, close, native readiness и concurrent capacity races.
 */
#include "backend.h"
#include "../../../runtime/vfs/launch/service.h"
#include <pthread.h>
static unsigned notices; /* Native scalar readiness count, updated atomically by publishers. */


/* EN/RU: Copy readiness only; no service/request/provider/guest pointers are retained or executed.
 * Только readiness, без service/request/provider/guest pointers и исполнения target. */
static void changed(void *state, const DVM_ChangeEvent *event)
{
    (void)event;
    __atomic_add_fetch((unsigned *)state, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Build copied request fields; roots remain borrowed through submit.
 * Сформировать поля запроса, roots borrowed до конца submit. */
static DVM_LaunchSpec spec(DVM_LaunchOptions *options, const char *name)
{
    DVM_LaunchSpec input = {0};
    input.policy = options->policy;
    input.terminal = options->terminal;
    input.terminal_origin = options->terminal_origin;
    strcpy(input.expected_provider, options->provider_origin.system);
    strcpy(input.module, name);
    return input;
}


/* EN/RU: Inspect immutable review label without taking or preparing its environment.
 * Прочитать immutable label без take/prepare окружения. */
static void label(DVM_LaunchReview *review, const char *expected)
{
    DVM_LaunchDescription description;
    launch_check(review && dvm_launch_describe(dvm_launch_review_request(review), &description) &&
        !strcmp(description.module, expected), "FIFO review order or request metadata");
}


/* EN/RU: Exercise quotas through queued/active/taken ownership and close all unfinished work.
 * Проверить quotas queued/active/taken ownership и close unfinished work. */
static void lifecycle(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 2);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_Changes *journal = dvm_launch_service_observe(owner);
    DVM_NoticeTap *tap = journal ? dvm_notice_subscribe(dvm_changes_notices(journal), changed, &notices, NULL) : NULL;
    launch_check(owner && endpoint && journal && tap, "service capabilities/observer");
    DVM_LaunchSpec first = spec(options, "first"), second = spec(options, "second"), third = spec(options, "third");
    first.expected_provider[0] = 'X';
    launch_check(!dvm_launch_service_submit(endpoint, &first), "wrong service destination accepted");
    first.expected_provider[0] = options->provider_origin.system[0];
    DVM_LaunchTicket *a = dvm_launch_service_submit(endpoint, &first);
    DVM_LaunchTicket *b = dvm_launch_service_submit(endpoint, &second);
    DVM_LaunchServiceInfo info;
    launch_check(a && b && !dvm_launch_service_submit(endpoint, &third) &&
        dvm_launch_service_info(owner, &info) && info.outstanding == 2 && info.queued == 2,
        "outstanding admission bound");
    dvm_launch_ticket_free(a);
    DVM_LaunchReview *review = dvm_launch_service_next(owner);
    label(review, "second");
    launch_check(dvm_launch_service_info(owner, &info) && info.outstanding == 1 && !info.queued,
                 "cancelled queue entry did not release quota");
    DVM_LaunchTicket *c = dvm_launch_service_submit(endpoint, &third);
    launch_check(c && !dvm_launch_service_submit(endpoint, &third), "reviewed request escaped quota");
    DVM_LaunchRequest *request = dvm_launch_review_request(review);
    launch_check(dvm_launch_request_prepare(request, options->provider, &options->provider_origin), "service review preparation");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff && dvm_launch_service_close(owner) && !dvm_launch_service_close(owner) &&
        !dvm_launch_service_next(owner) && !dvm_launch_service_endpoint(owner) &&
        !dvm_launch_service_submit(endpoint, &third) && dvm_launch_ticket_status(c) == DVM_LAUNCH_DENIED &&
        dvm_launch_ticket_status(b) == DVM_LAUNCH_TAKEN, "close revoked taken or retained pending work");
    launch_check(dvm_launch_service_info(owner, &info) && info.closed && !info.queued && info.outstanding == 2,
                 "close lost indexed ticket lifetime");
    dvm_launch_service_free(owner);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_ticket_free(b);
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(c);
    DVM_ResourcePlan path;
    FVM_FileInfo metadata;
    launch_check(dvm_resource_plan(handoff->group, "/server", 0, &path) &&
        dvm_resource_info(&path, &metadata) == FVM_OK, "handoff died with admission service");
    dvm_resource_close(&path);
    dvm_launch_handoff_free(handoff);
    launch_check(__atomic_load_n(&notices, __ATOMIC_RELAXED) > 0, "missing native readiness notifications");
    dvm_notice_unsubscribe(tap);
    DVM_ChangeCursor empty = {0};
    DVM_ChangeBatch last;
    launch_check(dvm_changes_pending(journal, &empty, DVM_CHANGE_ALL, &last) == 1 && last.event.generation > 1,
                 "observer journal retained service lifetime or lost final hints");
    dvm_changes_destroy(journal);
}


/* EN/RU: Borrow one shared endpoint/spec through concurrent submit, then join before freeing wrappers.
 * Заимствовать endpoint/spec для concurrent submit, join до освобождения wrappers. */
typedef struct Submitter
{
    DVM_LaunchEndpoint *endpoint; /* Borrowed immutable submit capability. */
    DVM_LaunchSpec *spec; /* Borrowed immutable spec. */
    DVM_LaunchTicket *ticket; /* Owned result or NULL at capacity/close. */
} Submitter;


/* EN/RU: Perform one native submission, never prepare or execute code.
 * Выполнить один native submit без prepare/init. */
static void *submit(void *state)
{
    Submitter *worker = state;
    worker->ticket = dvm_launch_service_submit(worker->endpoint, worker->spec);
    return NULL;
}


/* EN/RU: Eight simultaneous submitters cannot exceed limit1; repeat with owner close during submit.
 * Восемь submitters не превышают limit1; повторить с close во время submit. */
static void races(DVM_LaunchOptions *options)
{
    DVM_LaunchSpec input = spec(options, "concurrent-entry");
    for (unsigned round = 0; round < 32; round++)
    {
        DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
        DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
        launch_check(owner && endpoint, "race admission owner");
        Submitter workers[8] = {0};
        pthread_t threads[8];
        for (unsigned i = 0; i < 8; i++)
        {
            workers[i].endpoint = endpoint;
            workers[i].spec = &input;
            launch_check(!pthread_create(&threads[i], NULL, submit, &workers[i]), "submit worker construction");
        }
        if (round & 1)
        {
            dvm_launch_service_close(owner);
        }
        unsigned accepted = 0;
        for (unsigned i = 0; i < 8; i++)
        {
            launch_check(!pthread_join(threads[i], NULL), "submit worker join");
            accepted += workers[i].ticket != NULL;
        }
        launch_check(accepted <= 1 && ((round & 1) || accepted == 1), "concurrent capacity overrun");
        DVM_LaunchReview *review = dvm_launch_service_next(owner);
        if (!(round & 1))
        {
            label(review, "concurrent-entry");
        }
        dvm_launch_service_close(owner);
        for (unsigned i = 0; i < 8; i++)
        {
            if (workers[i].ticket)
            {
                launch_check(dvm_launch_ticket_status(workers[i].ticket) == DVM_LAUNCH_DENIED,
                             "closed admission retained runnable request");
                dvm_launch_ticket_free(workers[i].ticket);
            }
        }
        dvm_launch_review_free(review);
        DVM_LaunchServiceInfo info;
        launch_check(dvm_launch_service_info(owner, &info) && !info.outstanding && !info.queued,
                     "closed reservation/index quota leaked");
        dvm_launch_endpoint_free(endpoint);
        dvm_launch_service_free(owner);
    }
}


static pthread_mutex_t constructor_lock = PTHREAD_MUTEX_INITIALIZER; /* Controls only private constructor admission. */
static pthread_cond_t constructor_wake = PTHREAD_COND_INITIALIZER; /* Native test gate, no guest continuation. */
static int constructor_started; /* The slot reservation already exists. */
static int retirement_hold; /* Private finalization gate is enabled only for its exact scenario. */
static int retirement_started; /* Shared references reached zero, index removal has not run. */
static int retirement_continue; /* Main completed close while finalization was guarded. */
static int constructor_continue; /* Main closed the service, constructor may now complete. */
DVM_LaunchTicket *__real_dvm_launch_request_create(const DVM_LaunchSpec *, DVM_LaunchRequest **);


/* EN/RU: Hold only the reserved-constructor scenario after service reserve and before index publication.
 * Удержать только reserved constructor после reserve и до index publication, без timing sleeps. */
DVM_LaunchTicket *__wrap_dvm_launch_request_create(const DVM_LaunchSpec *input, DVM_LaunchRequest **out)
{
    if (!strcmp(input->module, "reserved"))
    {
        pthread_mutex_lock(&constructor_lock);
        constructor_started = 1;
        pthread_cond_broadcast(&constructor_wake);
        while (!constructor_continue)
        {
            pthread_cond_wait(&constructor_wake, &constructor_lock);
        }
        pthread_mutex_unlock(&constructor_lock);
    }
    return __real_dvm_launch_request_create(input, out);
}


/* EN/RU: Close during an actual reserved unpublished submission and verify complete rollback.
 * Закрыть сервис во время reserved unpublished submit и проверить полный rollback. */
static void reservation_close(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_LaunchSpec input = spec(options, "reserved");
    Submitter worker = {endpoint, &input, NULL};
    pthread_t thread;
    launch_check(owner && endpoint && !pthread_create(&thread, NULL, submit, &worker), "reserved submit construction");
    pthread_mutex_lock(&constructor_lock);
    while (!constructor_started)
    {
        pthread_cond_wait(&constructor_wake, &constructor_lock);
    }
    pthread_mutex_unlock(&constructor_lock);
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(owner, &info) && info.outstanding == 1 && !info.queued &&
        dvm_launch_service_close(owner), "reservation not counted or close failed");
    pthread_mutex_lock(&constructor_lock);
    constructor_continue = 1;
    pthread_cond_broadcast(&constructor_wake);
    pthread_mutex_unlock(&constructor_lock);
    launch_check(!pthread_join(thread, NULL) && !worker.ticket && dvm_launch_service_info(owner, &info) &&
        info.closed && !info.outstanding && !info.queued, "closed constructor published request or leaked reservation");
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
}


void __real_dvm_launch_slot_retired(void *context);


/* EN/RU: Gate only final index retirement; context remains owned by the finalizing native record.
 * Удержать только final index retirement; context принадлежит finalizing native record. */
void __wrap_dvm_launch_slot_retired(void *context)
{
    pthread_mutex_lock(&constructor_lock);
    if (retirement_hold)
    {
        retirement_started = 1;
        pthread_cond_broadcast(&constructor_wake);
        while (!retirement_continue)
        {
            pthread_cond_wait(&constructor_wake, &constructor_lock);
        }
    }
    pthread_mutex_unlock(&constructor_lock);
    __real_dvm_launch_slot_retired(context);
}


/* EN/RU: Consume one exclusive review on a joined worker, allowing final request retirement.
 * Поглотить exclusive review на worker, допускающем final request retirement. */
static void *free_review(void *review)
{
    dvm_launch_review_free(review);
    return NULL;
}


/* EN/RU: Close while a zero-reference indexed record is finalizing; no resurrection or stale access.
 * Закрыть при finalizing indexed record с refs0; без resurrection/stale access. */
static void finalization_close(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_LaunchSpec input = spec(options, "finalizing");
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &input);
    DVM_LaunchReview *review = dvm_launch_service_next(owner);
    launch_check(ticket && review, "finalization fixture");
    dvm_launch_ticket_free(ticket);
    pthread_mutex_lock(&constructor_lock);
    retirement_hold = 1;
    pthread_mutex_unlock(&constructor_lock);
    pthread_t worker;
    launch_check(!pthread_create(&worker, NULL, free_review, review), "finalization worker");
    pthread_mutex_lock(&constructor_lock);
    while (!retirement_started)
    {
        pthread_cond_wait(&constructor_wake, &constructor_lock);
    }
    pthread_mutex_unlock(&constructor_lock);
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_close(owner) && dvm_launch_service_info(owner, &info) &&
        info.closed && info.outstanding == 1 && !info.queued, "close resurrected finalizing index");
    pthread_mutex_lock(&constructor_lock);
    retirement_continue = 1;
    pthread_cond_broadcast(&constructor_wake);
    pthread_mutex_unlock(&constructor_lock);
    launch_check(!pthread_join(worker, NULL) && dvm_launch_service_info(owner, &info) && !info.outstanding,
                 "finalizing slot did not retire");
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
}


/* EN/RU: Run bounded owner/endpoint lifecycle and native concurrent admission races.
 * Проверить owner/endpoint lifecycle и native concurrent admission races. */
int main(void)
{
    LaunchState terminal = {0}, provider = {0};
    terminal.terminal = 1;
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    launch_check(!dvm_launch_service_create(options.provider_origin.system, 0) &&
        !dvm_launch_service_create(options.provider_origin.system, DVM_LAUNCH_SERVICE_LIMIT+1), "invalid service limits");
    lifecycle(&options);
    races(&options);
    reservation_close(&options);
    finalization_close(&options);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "service/root ownership leak");
    puts("PASS bounded launch service FIFO close quota races notices and handoff survival");
    return 0;
}
