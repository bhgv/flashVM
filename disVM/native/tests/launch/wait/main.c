/* Copyright (c) 2026 bhgv
 * EN: Real blocking admission, independent cancellation, queue-claim races and multi-handler close.
 * RU: Настоящий blocking admission, independent cancellation, queue-claim races и multi-handler close.
 */
#include "test.h"


/* EN/RU: Borrow roots/options to construct a copied valid request; return native value without taking root ownership.
 * Заимствовать roots/options для copied valid request; вернуть native value без переноса root ownership.
 */
static DVM_LaunchSpec input(DVM_LaunchOptions *options, const char *name)
{
    DVM_LaunchSpec spec = {0}; /* Plain request, no display required and no code executed by admission. */
    spec.policy = options->policy;
    spec.terminal = options->terminal;
    spec.terminal_origin = options->terminal_origin;
    strcpy(spec.expected_provider, options->provider_origin.system);
    strcpy(spec.module, name);
    return spec;
}


/* EN/RU: Inspect actual review metadata; review remains owned by caller, no preparation/module execution.
 * Прочитать actual review metadata; review принадлежит caller, без preparation/module execution.
 */
static void label(DVM_LaunchReview *review, const char *expected)
{
    DVM_LaunchDescription description;
    launch_check(review && dvm_launch_describe(dvm_launch_review_request(review), &description) &&
        !strcmp(description.module, expected), "blocking admission returned wrong request");
}


/* EN/RU: Require zero outstanding queue/review/ticket owners before closing and freeing caller-owned service/endpoint.
 * Требовать нулевой outstanding queue/review/ticket ownership до close/free service/endpoint.
 */
static void finish(DVM_LaunchService *owner, DVM_LaunchEndpoint *endpoint)
{
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(owner, &info) && !info.outstanding && !info.queued, "blocking admission quota drain");
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
}


/* EN/RU: Pre-cancel must preserve queued work; next wait returns exactly one request, never an entire queue.
 * Pre-cancel сохраняет queued work; следующий wait выдаёт ровно один request, не всю очередь.
 */
static void queued(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 2);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_LaunchSpec first = input(options, "first"); /* Independent copied labels demonstrate FIFO selection. */
    DVM_LaunchSpec second = input(options, "second");
    DVM_LaunchTicket *a = dvm_launch_service_submit(endpoint, &first);
    DVM_LaunchTicket *b = dvm_launch_service_submit(endpoint, &second);
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(owner && endpoint && a && b && cancel, "queued wait setup");
    dvm_cancel_request(cancel);
    launch_check(dvm_cancel_enter(&scope, cancel), "pre-cancel scope");
    DVM_LaunchReview *review = NULL; /* Public output remains NULL when no queue claim occurred. */
    launch_check(dvm_launch_service_wait(owner, &review) == DVM_LAUNCH_WAIT_CANCELLED && !review &&
        dvm_launch_ticket_status(a) == DVM_LAUNCH_PENDING, "pre-cancel consumed pending request");
    launch_check(dvm_cancel_leave(&scope), "pre-cancel scope release");
    dvm_cancel_release(cancel);
    launch_check(dvm_launch_service_wait(owner, &review) == DVM_LAUNCH_WAIT_REQUEST, "first queued wait");
    label(review, "first");
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(owner, &info) && info.queued == 1, "wait consumed more than one request");
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(a);
    launch_check(dvm_launch_service_wait(owner, &review) == DVM_LAUNCH_WAIT_REQUEST, "second queued wait");
    label(review, "second");
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(b);
    finish(owner, endpoint);
}


/* EN/RU: Check request, close and cancellation at a real native sleep or exact empty-check/sleep gap.
 * Проверить request, close и cancellation при native sleep либо в точном empty-check/sleep gap.
 */
static void wake(DVM_LaunchOptions *options, int action, int gap)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_Cancel *cancel = dvm_cancel_create();
    launch_check(owner && endpoint && cancel, "native wake setup");
    Waiter waiter = {.owner = owner, .cancel = cancel, .hold_empty = gap}; /* Native-only configured worker storage. */
    waiter_start(&waiter);
    waiter_await(&waiter, gap ? 1 : 0);
    DVM_LaunchTicket *ticket = NULL; /* Owned only in the publication branch. */
    if (action == 0)
    {
        DVM_LaunchSpec spec = input(options, "arrived");
        ticket = dvm_launch_service_submit(endpoint, &spec);
        launch_check(ticket != NULL, "native wake submit");
    }
    else if (action == 1)
    {
        dvm_launch_service_close(owner);
    }
    else
    {
        dvm_cancel_request(cancel);
    }
    if (gap)
    {
        waiter_release(&waiter, 1);
    }
    waiter_join(&waiter);
    DVM_LaunchWaitResult expected = action == 0 ? DVM_LAUNCH_WAIT_REQUEST : action == 1 ? DVM_LAUNCH_WAIT_CLOSED : DVM_LAUNCH_WAIT_CANCELLED;
    launch_check(waiter.result == expected && (action == 0 || !waiter.review), "native wait wake outcome");
    if (action == 0)
    {
        label(waiter.review, "arrived");
    }
    dvm_launch_review_free(waiter.review);
    dvm_launch_ticket_free(ticket);
    if (action == 2)
    {
        DVM_LaunchSpec spec = input(options, "still-open");
        ticket = dvm_launch_service_submit(endpoint, &spec);
        DVM_LaunchReview *review = NULL;
        launch_check(ticket && dvm_launch_service_wait(owner, &review) == DVM_LAUNCH_WAIT_REQUEST,
            "operation cancellation poisoned service or later wait");
        label(review, "still-open");
        dvm_launch_review_free(review);
        dvm_launch_ticket_free(ticket);
    }
    dvm_cancel_release(cancel);
    finish(owner, endpoint);
}


/* EN/RU: A completed dequeue remains owned despite later cancellation/close; no request replay/requeue.
 * Выполненный dequeue остаётся owned при последующей cancellation/close; без replay/requeue.
 */
static void claimed(DVM_LaunchOptions *options, int close)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_LaunchSpec spec = input(options, "claimed");
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &spec);
    DVM_Cancel *cancel = dvm_cancel_create();
    launch_check(owner && endpoint && ticket && cancel, "claimed race setup");
    Waiter waiter = {.owner = owner, .cancel = cancel, .hold_claim = 1}; /* Exact post-effect gate. */
    waiter_start(&waiter);
    waiter_await(&waiter, 2);
    if (close)
    {
        dvm_launch_service_close(owner);
    }
    else
    {
        dvm_cancel_request(cancel);
    }
    waiter_release(&waiter, 2);
    waiter_join(&waiter);
    launch_check(waiter.result == DVM_LAUNCH_WAIT_REQUEST, "acknowledged review discarded after cancellation/close");
    label(waiter.review, "claimed");
    launch_check(dvm_launch_ticket_status(ticket) == (close ? DVM_LAUNCH_DENIED : DVM_LAUNCH_PENDING),
        "wait cancellation changed request authority");
    dvm_launch_review_free(waiter.review);
    dvm_launch_ticket_free(ticket);
    dvm_cancel_release(cancel);
    finish(owner, endpoint);
}


/* EN/RU: Eight native handlers compete; cancelling seven independent waits leaves one live request recipient.
 * Восемь native handlers конкурируют; отмена семи independent waits оставляет одного получателя request.
 */
static void consumers(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    launch_check(owner && endpoint, "consumer setup");
    Waiter waiters[8] = {0}; /* Each has an independent cancellation scope and wake recipient. */
    for (unsigned i = 0; i < 8; i++)
    {
        waiters[i].owner = owner;
        waiters[i].cancel = dvm_cancel_create();
        launch_check(waiters[i].cancel != NULL, "consumer cancel allocation");
        waiter_start(&waiters[i]);
        waiter_await(&waiters[i], 0);
    }
    for (unsigned i = 0; i < 7; i++)
    {
        dvm_cancel_request(waiters[i].cancel);
        waiter_join(&waiters[i]);
        launch_check(waiters[i].result == DVM_LAUNCH_WAIT_CANCELLED && !waiters[i].review, "one cancellation affected another wait");
    }
    DVM_LaunchSpec spec = input(options, "survivor");
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &spec);
    launch_check(ticket != NULL, "surviving handler submit");
    waiter_join(&waiters[7]);
    launch_check(waiters[7].result == DVM_LAUNCH_WAIT_REQUEST, "unrelated handler stopped");
    label(waiters[7].review, "survivor");
    dvm_launch_review_free(waiters[7].review);
    dvm_launch_ticket_free(ticket);
    for (unsigned i = 0; i < 8; i++)
    {
        dvm_cancel_release(waiters[i].cancel);
    }
    finish(owner, endpoint);
}


/* EN/RU: Service close wakes every independent native handler; caller joins before owner free.
 * Service close будит все independent native handlers; caller делает join до owner free.
 */
static void close_all(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    launch_check(owner != NULL, "multi-close setup");
    Waiter waiters[8] = {0};
    for (unsigned i = 0; i < 8; i++)
    {
        waiters[i].owner = owner;
        waiter_start(&waiters[i]);
        waiter_await(&waiters[i], 0);
    }
    dvm_launch_service_close(owner);
    for (unsigned i = 0; i < 8; i++)
    {
        waiter_join(&waiters[i]);
        launch_check(waiters[i].result == DVM_LAUNCH_WAIT_CLOSED && !waiters[i].review, "close stranded native handler");
    }
    finish(owner, NULL);
}


/* EN/RU: One queued request has exactly one winner among eight waiting handlers, remaining handlers wake on close.
 * Один queued request имеет ровно одного победителя из восьми handlers, остальные пробуждаются при close.
 */
static void competition(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    launch_check(owner && endpoint, "competing handlers setup");
    Waiter waiters[8] = {0}; /* Same queue, independent native wake recipients. */
    for (unsigned i = 0; i < 8; i++)
    {
        waiters[i].owner = owner;
        waiter_start(&waiters[i]);
        waiter_await(&waiters[i], 0);
    }
    DVM_LaunchSpec spec = input(options, "one-winner");
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &spec);
    launch_check(ticket != NULL, "competing handlers submit");
    (void)waiter_any(waiters, 8);
    dvm_launch_service_close(owner);
    unsigned winners = 0; /* Count actual returned review owners, never just readiness notifications. */
    for (unsigned i = 0; i < 8; i++)
    {
        waiter_join(&waiters[i]);
        if (waiters[i].result == DVM_LAUNCH_WAIT_REQUEST)
        {
            winners++;
            label(waiters[i].review, "one-winner");
            dvm_launch_review_free(waiters[i].review);
        }
        else
        {
            launch_check(waiters[i].result == DVM_LAUNCH_WAIT_CLOSED && !waiters[i].review,
                "competing handler lost close or copied review");
        }
    }
    launch_check(winners == 1, "one request was delivered to multiple handlers");
    dvm_launch_ticket_free(ticket);
    finish(owner, endpoint);
}


/* EN/RU: Cancelled queue heads drain without being delivered, the next pending request preserves FIFO metadata.
 * Отменённый queue head удаляется без выдачи handler, следующий pending request сохраняет FIFO metadata.
 */
static void cancelled_head(DVM_LaunchOptions *options)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 2);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_LaunchSpec first = input(options, "cancelled-head");
    DVM_LaunchSpec second = input(options, "pending-tail");
    DVM_LaunchTicket *a = dvm_launch_service_submit(endpoint, &first);
    DVM_LaunchTicket *b = dvm_launch_service_submit(endpoint, &second);
    DVM_LaunchReview *review = NULL;
    launch_check(a && b && dvm_launch_ticket_cancel(a) &&
        dvm_launch_service_wait(owner, &review) == DVM_LAUNCH_WAIT_REQUEST, "cancelled queue-head wait");
    label(review, "pending-tail");
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(a);
    dvm_launch_ticket_free(b);
    finish(owner, endpoint);
}


/* EN/RU: Run real native queue/wait cases with deterministic barriers; no provider/guest execution is synthesized.
 * Выполнить настоящие native queue/wait cases с deterministic barriers; provider/guest execution не подменяется.
 */
int main(void)
{
    LaunchState terminal = {0};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_LaunchReview *review = NULL;
    launch_check(dvm_launch_service_wait(NULL, &review) == DVM_LAUNCH_WAIT_FAILED && !review,
        "invalid wait input accepted");
    queued(&options);
    cancelled_head(&options);
    for (int gap = 0; gap < 2; gap++)
    {
        for (int action = 0; action < 3; action++)
        {
            wake(&options, action, gap);
        }
    }
    claimed(&options, 0);
    claimed(&options, 1);
    consumers(&options);
    close_all(&options);
    for (unsigned round = 0; round < 32; round++)
    {
        competition(&options);
    }
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "blocking admission root ownership drain");
    puts("PASS native blocking launch admission and independent handler cancellation");
    return 0;
}
