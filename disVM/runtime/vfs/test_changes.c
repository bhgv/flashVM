/* Copyright (c) 2026 bhgv
 * EN: Independent cursors, scoped coalescing, refresh races and concurrent hints.
 * RU: Независимые cursors, scoped coalescing, refresh races и concurrent hints.
 */
#include "changes.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHANGE_TEST_GENERATIONS 5000U /* Concurrent producer's final generation. */


/* EN/RU: Caller-owned refresh context; no state shared with producer thread.
 * Caller-owned refresh context, без shared state с producer thread. */
typedef struct Refresh
{
    DVM_Changes *changes; /* Borrowed live journal for reentrant notifications. */
    unsigned calls; /* Number of actual state queries, not observations. */
    int action; /* Zero normal, one refusal, two newer event, three reconnect. */
} Refresh;


/* EN/RU: Fail condition with native diagnostic, independent from assert modes.
 * Завершить при false с native diagnostic, без assert modes. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "provider changes: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Publish one report, requiring its exact accept/ignore/refuse result.
 * Опубликовать report и проверить exact result. Inputs are scalar test values. */
static void notify(DVM_Changes *changes, uint64_t session, uint64_t generation,
                   unsigned kinds, unsigned flags, uint64_t resource, int expected)
{
    DVM_ChangeEvent event = {session, generation, kinds, flags, resource};
    require(dvm_changes_notify(changes, &event) == expected, "notification result");
}


/* EN/RU: Simulate a state query, optionally racing a new event/epoch or refusing.
 * Callback reenters journal: holding its lock across callbacks would deadlock.
 * Имитировать query с новым event/epoch либо refusal. Reentry проверяет unlocked callback. */
static FVM_Result refresh(const DVM_ChangeBatch *ticket, void *argument)
{
    Refresh *context = argument;
    context->calls++;
    require(ticket->event.kinds != 0, "empty refresh callback");
    if (context->action == 1)
    {
        return FVM_ERROR;
    }
    if (context->action == 2)
    {
        notify(context->changes, ticket->event.session, ticket->event.generation + 1,
               DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 7, 1);
    }
    if (context->action == 3)
    {
        require(dvm_changes_reconnect(context->changes, ticket->event.session) != 0, "callback reconnect");
    }
    return FVM_OK;
}


/* EN/RU: Refresh every pending class once; caller owns cursor and context.
 * Перечитать все pending classes один раз; caller владеет cursor/context. */
static void drain(DVM_Changes *changes, DVM_ChangeCursor *cursor, Refresh *context)
{
    context->action = 0;
    require(dvm_changes_refresh(changes, cursor, DVM_CHANGE_ALL, refresh, context) == FVM_OK, "drain");
}


/* EN/RU: Publish a contiguous session-one stream from its own OS thread.
 * Публиковать contiguous session-one stream из отдельного OS thread. */
static void *producer(void *argument)
{
    DVM_Changes *changes = argument;
    unsigned generation;
    for (generation = 2; generation <= CHANGE_TEST_GENERATIONS; generation++)
    {
        notify(changes, 1, generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, generation % 3, 1);
    }
    return NULL;
}


/* EN/RU: Concurrent producer cannot lose updates when refresh acknowledges tickets.
 * After join, all events can be drained; an independent observer is still dirty.
 * Concurrent producer не теряет updates при ack tickets; другой observer не очищается. */
static void concurrent(void)
{
    DVM_Changes *changes = dvm_changes_create();
    DVM_ChangeCursor cursor = {0}, other = {0};
    DVM_ChangeBatch batch;
    Refresh context = {changes, 0, 0};
    pthread_t thread;
    unsigned index;
    require(changes != NULL, "concurrent journal");
    drain(changes, &cursor, &context);
    require(!pthread_create(&thread, NULL, producer, changes), "start producer");
    for (index = 0; index < CHANGE_TEST_GENERATIONS; index++)
    {
        drain(changes, &cursor, &context);
    }
    require(!pthread_join(thread, NULL), "join producer");
    drain(changes, &cursor, &context);
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 0 &&
            batch.event.generation == CHANGE_TEST_GENERATIONS, "concurrent lost event");
    require(dvm_changes_pending(changes, &other, DVM_CHANGE_ALL, &batch) == 1, "observer globally cleared");
    dvm_changes_destroy(changes);
}


/* EN/RU: Exercise selective ack, resource coalescing, generation gaps, journal
 * overflow, disconnect/reconnect and changes arriving inside refresh callbacks.
 * Проверить selective ack, scope, gaps, overflow, reconnect и callback races. */
int main(void)
{
    DVM_Changes *changes = dvm_changes_create();
    DVM_ChangeCursor cursor = {0}, other = {0}, before;
    DVM_ChangeBatch batch;
    Refresh context = {changes, 0, 0};
    unsigned calls, index;
    uint64_t session;
    require(changes != NULL && dvm_changes_session(changes) == 1, "initial journal");
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
            batch.event.kinds == DVM_CHANGE_ALL && !batch.event.flags, "initial full refresh");
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_RIGHTS, refresh, &context) == FVM_OK, "rights refresh");
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_RIGHTS, &batch) == 0 &&
            dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
            batch.event.kinds == (DVM_CHANGE_ALL & ~DVM_CHANGE_RIGHTS), "selective refresh cleared other classes");
    before = cursor;
    context.action = 1;
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, refresh, &context) == FVM_ERROR &&
            !memcmp(&before, &cursor, sizeof(cursor)), "failed query advanced cursor");
    drain(changes, &cursor, &context);
    calls = context.calls;
    drain(changes, &cursor, &context);
    require(context.calls == calls, "clean observation queried provider");
    require(dvm_changes_pending(changes, &other, DVM_CHANGE_ALL, &batch) == 1, "independent cursor cleared");
    other = cursor;
    notify(changes, 1, 2, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 0, 1);
    notify(changes, 1, 3, DVM_CHANGE_RIGHTS, DVM_CHANGE_RESOURCE, 0, 1);
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
            batch.event.flags == DVM_CHANGE_RESOURCE && batch.event.resource == 0, "resource zero/coalescing");
    notify(changes, 1, 4, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 99, 1);
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 && !batch.event.flags, "mixed resources narrowed");
    notify(changes, 1, 4, DVM_CHANGE_CONTENT, 0, 0, 0);
    notify(changes, 1, 3, DVM_CHANGE_RIGHTS, 0, 0, 0);
    notify(changes, 1, 5, DVM_CHANGE_LOAD, DVM_CHANGE_RESOURCE, 1, -1);
    notify(changes, 2, 5, DVM_CHANGE_CONNECTION, 0, 0, -1);
    context.action = 2;
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, refresh, &context) == FVM_OK, "refresh with new event");
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
            batch.event.kinds == DVM_CHANGE_CONTENT && batch.event.resource == 7, "new event lost during refresh");
    drain(changes, &cursor, &context);
    require(dvm_changes_pending(changes, &other, DVM_CHANGE_ALL, &batch) == 1, "copied cursor globally advanced");
    notify(changes, 1, 6, DVM_CHANGE_CONNECTION, 0, 0, 1);
    before = cursor;
    context.action = 3;
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, refresh, &context) == FVM_NOT_FOUND &&
            !memcmp(&before, &cursor, sizeof(cursor)), "stale-epoch query acknowledged");
    session = dvm_changes_session(changes);
    require(session == 2 && !dvm_changes_reconnect(changes, 1), "stale reconnect replaced session");
    notify(changes, 1, 100, DVM_CHANGE_ALL, 0, 0, 0);
    drain(changes, &cursor, &context);
    notify(changes, session, 5, DVM_CHANGE_LOAD, 0, 0, 1);
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
            batch.event.kinds == DVM_CHANGE_ALL && !batch.event.flags, "gap hid missing classes");
    drain(changes, &cursor, &context);
    for (index = 6; index < 6 + DVM_CHANGE_HISTORY + 2; index++)
    {
        notify(changes, session, index, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 8, 1);
    }
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_CONTENT, &batch) == 1 && !batch.event.flags,
            "evicted history narrowed scope");
    drain(changes, &cursor, &context);
    notify(changes, session, UINT64_MAX, DVM_CHANGE_NETWORK, 0, 0, 1);
    notify(changes, session, UINT64_MAX, DVM_CHANGE_NETWORK, 0, 0, 0);
    require(dvm_changes_reconnect(changes, session) == session + 1, "generation exhaustion recovery");
    dvm_changes_destroy(changes);
    concurrent();
    puts("PASS provider changes selective/copy/races/gaps/overflow/reconnect/resource scope and 4999 concurrent reports");
    return 0;
}
