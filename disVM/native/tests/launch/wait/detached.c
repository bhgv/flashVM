/* Copyright (c) 2026 bhgv
 * EN: An already admitted readiness callback may finish after cancelled wait/owner retirement, without stale native state.
 * RU: Уже принятый readiness callback завершается после отменённого wait/owner retirement без stale native state.
 */
#include "../backend.h"
#include "../../../../runtime/vfs/launch/service.h"
#include <pthread.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Fixture-only callback/thread rendezvous. */
static pthread_cond_t progress = PTHREAD_COND_INITIALIZER; /* Deterministic native wake without timing sleeps. */
static __thread int waiter_thread; /* Select only this public wait's first journal subscription. */
static int sleeping; /* Actual wait reached native sleep, protected by gate. */
static int entered; /* Journal callback admitted but not yet delivered to actual recipient. */
static int proceed; /* Main permits the held callback to deliver to independently retained state. */
static int freed; /* Actual recipient allocation has reached free, gate-protected. */
static void *recipient; /* Native wait allocation identity, never dereferenced by fixture. */
static DVM_NoticeReceive receive_callback; /* Actual journal callback, preserved unchanged. */
static DVM_NoticeRelease release_callback; /* Actual consuming recipient release, preserved unchanged. */


/* EN/RU: Original subscribe owns state only on success; native dispatcher pins callbacks until retirement.
 * Исходный subscribe принимает state лишь при success; dispatcher удерживает callbacks до retirement.
 */
extern DVM_NoticeTap *__real_dvm_notice_subscribe(DVM_NoticeHub *, DVM_NoticeReceive, void *, DVM_NoticeRelease);


/* EN/RU: Original native wait borrows locked sync, returning with the same mutex ownership.
 * Исходный native wait заимствует locked sync, возвращая тот же mutex ownership.
 */
extern void __real_fvm_port_sync_wait(FVM_PortSync *);


/* EN/RU: Original native free consumes allocation/NULL; no wrapper recursion.
 * Исходный native free поглощает allocation/NULL; без wrapper-рекурсии.
 */
extern void __real_free(void *);


/* EN/RU: Observe actual recipient free, preserving native allocator semantics and all unrelated frees.
 * Наблюдать настоящий recipient free, сохраняя native allocator semantics и остальные frees.
 */
void __wrap_free(void *value)
{
    pthread_mutex_lock(&gate);
    if (value && value == recipient)
    {
        freed++;
    }
    pthread_mutex_unlock(&gate);
    __real_free(value);
}


/* EN/RU: Hold only an admitted journal callback before actual delivery; no production mutex is held by this gate.
 * Удержать лишь admitted journal callback до actual delivery; production mutex этим gate не удерживается.
 */
static void held(void *state, const DVM_ChangeEvent *event)
{
    pthread_mutex_lock(&gate);
    entered = 1;
    pthread_cond_broadcast(&progress);
    while (!proceed)
    {
        pthread_cond_wait(&progress, &gate);
    }
    pthread_mutex_unlock(&gate);
    receive_callback(state, event); /* Actual callback must still have a live native recipient/condition. */
}


/* EN/RU: Intercept only the wait's journal tap; cancellation tap stays real and can wake independently.
 * Перехватить лишь journal tap ожидания; cancellation tap остаётся настоящим и будит независимо.
 */
DVM_NoticeTap *__wrap_dvm_notice_subscribe(DVM_NoticeHub *hub, DVM_NoticeReceive receive, void *state, DVM_NoticeRelease release)
{
    if (waiter_thread && !recipient)
    {
        pthread_mutex_lock(&gate);
        recipient = state;
        receive_callback = receive;
        release_callback = release;
        pthread_mutex_unlock(&gate);
        return __real_dvm_notice_subscribe(hub, held, state, release);
    }
    return __real_dvm_notice_subscribe(hub, receive, state, release);
}


/* EN/RU: Publish imminent actual condition wait to main; no result/sleep is simulated.
 * Сообщить main о imminent actual condition wait; result/sleep не подменяются.
 */
void __wrap_fvm_port_sync_wait(FVM_PortSync *sync)
{
    if (waiter_thread)
    {
        pthread_mutex_lock(&gate);
        sleeping = 1;
        pthread_cond_broadcast(&progress);
        pthread_mutex_unlock(&gate);
    }
    __real_fvm_port_sync_wait(sync);
}


/* EN/RU: Waiter borrows native service/source through join; result conveys no review on cancellation.
 * Waiter заимствует native service/source до join; result не содержит review при cancellation.
 */
typedef struct Waiter
{
    DVM_LaunchService *owner; /* Borrowed service through wait return/join. */
    DVM_Cancel *cancel; /* Borrowed per-operation source through scope release/join. */
    DVM_LaunchWaitResult result; /* Actual public API status after join. */
    DVM_LaunchReview *review; /* Actual public API owner/NULL after join. */
} Waiter;


/* EN/RU: Enter real native scope and wait; callback may still be admitted when this thread returns.
 * Войти в настоящий native scope и wait; callback может оставаться admitted после возврата потока.
 */
static void *wait(void *state)
{
    Waiter *worker = state;
    DVM_CancelScope scope = {0};
    waiter_thread = 1;
    launch_check(dvm_cancel_enter(&scope, worker->cancel), "detached wait scope");
    worker->result = dvm_launch_service_wait(worker->owner, &worker->review);
    launch_check(dvm_cancel_leave(&scope), "detached wait scope retirement");
    waiter_thread = 0;
    return NULL;
}


/* EN/RU: Publisher borrows submit capability/spec through join; actual submit pauses only inside the test callback gate.
 * Publisher заимствует submit capability/spec до join; настоящий submit задержан лишь test callback gate.
 */
typedef struct Publisher
{
    DVM_LaunchEndpoint *endpoint; /* Borrowed native capability, stays live through join. */
    DVM_LaunchSpec *spec; /* Borrowed immutable request inputs. */
    DVM_LaunchTicket *ticket; /* Actual returned requester capability after publisher join. */
} Publisher;


/* EN/RU: Submit real request, retaining its normal queue/slot lifetime through synchronous journal notification.
 * Подать настоящий request, сохраняя обычный queue/slot lifetime во время synchronous journal notification.
 */
static void *publish(void *state)
{
    Publisher *worker = state;
    worker->ticket = dvm_launch_service_submit(worker->endpoint, worker->spec);
    return NULL;
}


/* EN/RU: Block one admitted callback, cancel/join wait and retire owner first, then require valid late delivery and exact free.
 * Заблокировать admitted callback, cancel/join wait и освободить owner раньше; требовать valid late delivery и точный free.
 */
int main(void)
{
    LaunchState terminal = {0};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_LaunchService *owner = dvm_launch_service_create(options.provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_Cancel *cancel = dvm_cancel_create();
    launch_check(owner && endpoint && cancel, "detached callback setup");
    Waiter waiter = {.owner = owner, .cancel = cancel};
    pthread_t waiter_id; /* Joined before owner free; source callbacks own independent native state. */
    launch_check(!pthread_create(&waiter_id, NULL, wait, &waiter), "detached waiter start");
    pthread_mutex_lock(&gate);
    while (!sleeping)
    {
        pthread_cond_wait(&progress, &gate);
    }
    pthread_mutex_unlock(&gate);
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "detached-callback");
    Publisher publisher = {.endpoint = endpoint, .spec = &spec};
    pthread_t publisher_id; /* Retains borrowed endpoint/spec until its late callback returns. */
    launch_check(!pthread_create(&publisher_id, NULL, publish, &publisher), "detached publisher start");
    pthread_mutex_lock(&gate);
    while (!entered)
    {
        pthread_cond_wait(&progress, &gate);
    }
    pthread_mutex_unlock(&gate);
    dvm_cancel_request(cancel);
    launch_check(!pthread_join(waiter_id, NULL) && waiter.result == DVM_LAUNCH_WAIT_CANCELLED && !waiter.review,
        "wait joined callback or consumed request after cancellation");
    dvm_launch_service_free(owner); /* No live waiter borrows owner; publisher still holds its separate endpoint pin. */
    dvm_cancel_release(cancel);
    pthread_mutex_lock(&gate);
    launch_check(recipient && receive_callback && release_callback && !freed, "admitted callback lost native recipient pin");
    proceed = 1;
    pthread_cond_broadcast(&progress);
    pthread_mutex_unlock(&gate);
    launch_check(!pthread_join(publisher_id, NULL) && publisher.ticket &&
        dvm_launch_ticket_status(publisher.ticket) == DVM_LAUNCH_DENIED, "late callback changed closed request authority");
    pthread_mutex_lock(&gate);
    launch_check(freed == 1, "detached callback did not retire exactly one native recipient");
    pthread_mutex_unlock(&gate);
    dvm_launch_ticket_free(publisher.ticket);
    dvm_launch_endpoint_free(endpoint);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "detached callback root drain");
    puts("PASS native launch wait returns before admitted callback retirement");
    return 0;
}
