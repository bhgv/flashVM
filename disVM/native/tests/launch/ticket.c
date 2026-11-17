/* Copyright (c) 2026 bhgv
 * EN: Separated launch authority, immutable startup bytes, cancellation and independent handoff lifetime.
 * RU: Раздельные launch authority, immutable startup bytes, отмена и независимый handoff lifetime.
 */
#include "backend.h"
#include "../../../runtime/vfs/launch/ticket.h"
#include <pthread.h>


/* EN/RU: Borrow request through joined concurrent take; result belongs to this worker.
 * Заимствовать request до join concurrent take; result принадлежит worker. */
typedef struct Taker
{
    DVM_LaunchRequest *request; /* Borrowed through join. */
    DVM_LaunchHandoff *result; /* Owned winner's environment, NULL for loser. */
} Taker;


/* EN/RU: Race one provider take using stable borrowed wrapper; return no thread payload.
 * Выполнить provider take с borrowed wrapper; thread payload отсутствует. */
static void *take(void *opaque)
{
    Taker *taker = opaque;
    taker->result = dvm_launch_request_take(taker->request);
    return NULL;
}


/* EN/RU: Copy options into a request spec with mutable caller bytes; no native I/O.
 * Скопировать options в spec с mutable caller bytes, без native I/O. */
static DVM_LaunchSpec spec(DVM_LaunchOptions *options, unsigned char *bytes)
{
    DVM_LaunchSpec result = {0};
    result.policy = options->policy;
    result.terminal = options->terminal;
    result.terminal_origin = options->terminal_origin;
    strcpy(result.expected_provider, options->provider_origin.system);
    strcpy(result.module, "logical-module");
    result.arguments = bytes;
    result.argument_bytes = 4;
    return result;
}


/* EN/RU: Check each terminal state and requester/provider abandonment before/after preparation.
 * Проверить terminal states и requester/provider abandonment до/после preparation. */
static void lifecycle(DVM_LaunchOptions *options, int kind)
{
    unsigned char bytes[] = {0, 1, 2, 255};
    DVM_LaunchSpec input = spec(options, bytes);
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&input, &request);
    launch_check(ticket && request && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING,
                 "request publication");
    bytes[0] = 99;
    memset(input.module, 'X', sizeof(input.module));
    DVM_LaunchDescription description;
    unsigned char copied[8] = {0};
    size_t count = 0;
    launch_check(dvm_launch_describe(request, &description) && !strcmp(description.module, "logical-module") &&
        dvm_launch_arguments(request, 0, copied, sizeof(copied), &count) && count == 4 && copied[0] == 0 &&
        copied[3] == 255 && dvm_launch_arguments(request, 4, NULL, 0, &count) && count == 0 &&
        !dvm_launch_arguments(request, 5, copied, sizeof(copied), &count), "mutable or unbounded request bytes");
    DVM_Changes *changes = dvm_launch_observe(ticket);
    launch_check(changes != NULL, "independent observer journal");
    DVM_ChangeCursor empty = {0};
    DVM_ChangeBatch before, after;
    launch_check(dvm_changes_pending(changes, &empty, DVM_CHANGE_ALL, &before) == 1, "initial state journal");
    DVM_Origin wrong = options->provider_origin;
    wrong.system[0] = 'X';
    launch_check(!dvm_launch_request_prepare(request, options->provider, &wrong) &&
        dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING, "wrong destination changed state");
    wrong = options->provider_origin;
    wrong.session[0] = 'X';
    launch_check(!dvm_launch_request_prepare(request, options->provider, &wrong) &&
        dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING, "foreign session changed state");
    if (kind == 1)
    {
        launch_check(dvm_launch_ticket_cancel(ticket) && !dvm_launch_ticket_cancel(ticket), "pending cancel");
    }
    else if (kind == 2)
    {
        launch_check(dvm_launch_request_refuse(request), "provider refuse");
    }
    else if (kind == 3)
    {
        dvm_launch_request_free(request);
        request = NULL;
        launch_check(dvm_launch_ticket_status(ticket) == DVM_LAUNCH_DENIED, "provider abandonment");
    }
    else if (kind == 4)
    {
        dvm_launch_ticket_free(ticket);
        ticket = NULL;
        launch_check(!dvm_launch_request_prepare(request, options->provider, &options->provider_origin),
                     "requester abandonment did not cancel");
    }
    else
    {
        launch_check(dvm_launch_request_prepare(request, options->provider, &options->provider_origin) &&
            dvm_launch_ticket_status(ticket) == DVM_LAUNCH_READY &&
            !dvm_launch_request_prepare(request, options->provider, &options->provider_origin), "single prepare");
        if (kind == 5)
        {
            launch_check(dvm_launch_ticket_cancel(ticket), "ready cancel");
        }
        else if (kind == 6)
        {
            launch_check(dvm_launch_request_refuse(request), "ready refuse");
        }
        else
        {
            Taker takers[2] = {{request, NULL}, {request, NULL}};
            pthread_t threads[2];
            launch_check(!pthread_create(&threads[0], NULL, take, &takers[0]) &&
                !pthread_create(&threads[1], NULL, take, &takers[1]), "take race construction");
            launch_check(!pthread_join(threads[0], NULL) && !pthread_join(threads[1], NULL), "take race join");
            launch_check((takers[0].result != NULL) != (takers[1].result != NULL), "environment issued twice");
            DVM_LaunchHandoff *handoff = takers[0].result ? takers[0].result : takers[1].result;
            launch_check(dvm_launch_ticket_status(ticket) == DVM_LAUNCH_TAKEN &&
                !dvm_launch_ticket_cancel(ticket) && !dvm_launch_request_refuse(request), "taken environment revoked");
            dvm_launch_ticket_free(ticket);
            dvm_launch_request_free(request);
            ticket = NULL;
            request = NULL;
            DVM_ResourcePlan path;
            FVM_FileInfo metadata;
            launch_check(handoff->arguments[0] == 0 && handoff->arguments[3] == 255 &&
                !strcmp(handoff->description.module, "logical-module") &&
                dvm_resource_plan(handoff->group, "/dev/camera", 0, &path) &&
                dvm_resource_info(&path, &metadata) == FVM_OK && path.origin.role == DVM_ORIGIN_TERMINAL,
                "handoff depended on ticket/request lifetime");
            dvm_resource_close(&path);
            dvm_launch_handoff_free(handoff);
        }
    }
    if (request)
    {
        launch_check(!dvm_launch_request_take(request), "terminal request issued environment");
    }
    launch_check(dvm_changes_pending(changes, &empty, DVM_CHANGE_ALL, &after) == 1 &&
        after.event.generation > before.event.generation, "state change was not published");
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_changes_destroy(changes);
}


/* EN/RU: Native gate starts take/cancel together; opposite capabilities stay owned through join.
 * Native gate одновременно допускает take/cancel; wrappers удержаны до join. */
typedef struct CommitRace
{
    pthread_mutex_t lock; /* Protects launch gate only. */
    pthread_cond_t wake; /* Native start notification. */
    int go; /* Both operations may proceed. */
    DVM_LaunchTicket *ticket; /* Borrowed requester capability. */
    DVM_LaunchRequest *request; /* Borrowed provider capability. */
    DVM_LaunchHandoff *handoff; /* Take worker's owned result. */
    int cancelled; /* Cancel worker's result. */
} CommitRace;


/* EN/RU: Wait for native start with no target guest or timing sleeps.
 * Ждать native start без guest и timing sleeps. */
static void start(CommitRace *race)
{
    pthread_mutex_lock(&race->lock);
    while (!race->go)
    {
        pthread_cond_wait(&race->wake, &race->lock);
    }
    pthread_mutex_unlock(&race->lock);
}


/* EN/RU: Attempt one requester cancellation after the shared native gate opens.
 * Попытаться отменить запрос после shared native gate. */
static void *cancel_race(void *opaque)
{
    CommitRace *race = opaque;
    start(race);
    race->cancelled = dvm_launch_ticket_cancel(race->ticket);
    return NULL;
}


/* EN/RU: Attempt one provider handoff after the shared native gate opens.
 * Попытаться выдать окружение после shared native gate. */
static void *take_race(void *opaque)
{
    CommitRace *race = opaque;
    start(race);
    race->handoff = dvm_launch_request_take(race->request);
    return NULL;
}


/* EN/RU: Run64 ready/take/cancel races; exactly one decision wins and roots remain owned.
 * Проверить64 races ready/take/cancel; ровно одно решение побеждает, roots остаются owned. */
static void commit_races(DVM_LaunchOptions *options)
{
    unsigned char bytes[4] = {0, 1, 2, 3};
    DVM_LaunchSpec input = spec(options, bytes);
    for (unsigned round = 0; round < 64; round++)
    {
        CommitRace race = {0};
        race.ticket = dvm_launch_request_create(&input, &race.request);
        launch_check(race.ticket && dvm_launch_request_prepare(race.request, options->provider,
            &options->provider_origin), "race preparation");
        launch_check(!pthread_mutex_init(&race.lock, NULL) && !pthread_cond_init(&race.wake, NULL), "race gate");
        pthread_t workers[2];
        launch_check(!pthread_create(&workers[0], NULL, take_race, &race) &&
            !pthread_create(&workers[1], NULL, cancel_race, &race), "race workers");
        pthread_mutex_lock(&race.lock);
        race.go = 1;
        pthread_cond_broadcast(&race.wake);
        pthread_mutex_unlock(&race.lock);
        launch_check(!pthread_join(workers[0], NULL) && !pthread_join(workers[1], NULL), "race worker join");
        launch_check(race.handoff ? !race.cancelled && dvm_launch_ticket_status(race.ticket) == DVM_LAUNCH_TAKEN :
            race.cancelled && dvm_launch_ticket_status(race.ticket) == DVM_LAUNCH_CANCELLED,
            "cancel and take both won or lost ownership");
        dvm_launch_ticket_free(race.ticket);
        dvm_launch_request_free(race.request);
        dvm_launch_handoff_free(race.handoff);
        pthread_cond_destroy(&race.wake);
        pthread_mutex_destroy(&race.lock);
    }
}


/* EN/RU: Exercise malformed specs, independent lifecycle scenarios and exact root retirement.
 * Проверить invalid specs, lifecycle scenarios и точное освобождение roots. */
int main(void)
{
    LaunchState terminal = {0}, provider = {0};
    terminal.terminal = 1;
    terminal.value = 'T';
    provider.value = 'P';
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    unsigned char bytes[4] = {0};
    DVM_LaunchSpec input = spec(&options, bytes);
    DVM_LaunchRequest *request = NULL;
    input.argument_bytes = DVM_LAUNCH_ARGUMENT_LIMIT+1;
    launch_check(!dvm_launch_request_create(&input, &request) && !request, "oversized startup bytes accepted");
    LaunchState legacy = {0};
    DVM_Provider *legacy_root = dvm_provider_create_native(&launch_ops, &legacy, NULL, 1, 0);
    launch_check(legacy_root != NULL, "legacy fixture root");
    input = spec(&options, bytes);
    input.terminal = legacy_root;
    launch_check(!dvm_launch_request_create(&input, &request) && !request && !legacy.roots,
                 "legacy root received cancellation promise or metadata callback");
    dvm_provider_release(legacy_root);
    launch_check(legacy.destroyed == 1, "legacy refusal lifetime");
    input = spec(&options, bytes);
    input.argument_bytes = 0;
    input.arguments = NULL;
    DVM_LaunchTicket *empty = dvm_launch_request_create(&input, &request);
    launch_check(empty && dvm_launch_request_prepare(request, options.provider, &options.provider_origin),
                 "zero-argument preparation");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff && !handoff->arguments && !handoff->description.argument_bytes, "zero-argument handoff");
    dvm_launch_ticket_free(empty);
    dvm_launch_request_free(request);
    dvm_launch_handoff_free(handoff);
    input = spec(&options, bytes);
    provider.file_root = 1;
    DVM_LaunchTicket *bad = dvm_launch_request_create(&input, &request);
    launch_check(bad && !dvm_launch_request_prepare(request, options.provider, &options.provider_origin) &&
        dvm_launch_ticket_status(bad) == DVM_LAUNCH_DENIED && !dvm_launch_request_take(request),
        "wrong root kind published ready");
    dvm_launch_ticket_free(bad);
    dvm_launch_request_free(request);
    provider.file_root = 0;
    for (int kind = 0; kind < 7; kind++)
    {
        lifecycle(&options, kind);
    }
    commit_races(&options);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "launch ticket root leak");
    puts("PASS launch ticket authority lifecycle copied bytes one-use handoff and notices");
    return 0;
}
