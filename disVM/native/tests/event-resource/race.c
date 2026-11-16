/* Copyright (c) 2026 bhgv
 * EN: Deterministic in-flight teardown, reordered native publications and shared-reader tests.
 * RU: Детерминированные in-flight teardown, reordered native publications и shared-reader tests.
 */
#include "events_provider.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: External fixture owns storage until all native publishers/readers join.
 * External fixture хранит storage до join всех native publishers/readers. */
typedef struct State
{
    pthread_mutex_t lock; /* Fixture counters/gate only. */
    pthread_cond_t gate; /* Deterministic producer admission. */
    unsigned long long block; /* Generation held in allow callback,0 disabled. */
    int entered; /* Blocked callback admitted. */
    int proceed; /* Fixture gate permits return. */
    unsigned released; /* Policy cleanup after final callback retirement. */
    unsigned total; /* Shared-reader completed records. */
    unsigned long long seen; /* Duplicate detection for small fixture generation range. */
    DVM_Changes *source; /* Borrowed main journal pin through join. */
    StyxEvents *events; /* Borrowed main owner pin through reader join. */
} State;


/* EN/RU: Check independently of native build mode.
 * Проверить независимо от native build mode. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "event resource race: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Callback blocks only in this controlled fixture, exposing admitted publisher lifetime.
 * Callback блокируется только в fixture для проверки admitted publisher lifetime. */
static int allow(void *opaque, const StyxEvent *event)
{
    State *state = opaque;
    pthread_mutex_lock(&state->lock);
    if (event->generation == state->block)
    {
        state->entered = 1;
        pthread_cond_broadcast(&state->gate);
        while (!state->proceed)
        {
            pthread_cond_wait(&state->gate, &state->lock);
        }
    }
    pthread_mutex_unlock(&state->lock);
    return 1;
}


/* EN/RU: Fixture-only native scalar copy, no public event encoding implied.
 * Только fixture native scalar copy, без public event encoding. */
static int encode(void *opaque, const StyxEvent *event, void *bytes, size_t size, size_t *count)
{
    (void)opaque;
    if (size < sizeof(*event))
    {
        return 0;
    }
    memcpy(bytes, event, sizeof(*event));
    *count = sizeof(*event);
    return 1;
}


/* EN/RU: State storage remains externally held, so release can be inspected after thread join.
 * State storage удерживается извне, release проверяется после thread join. */
static void release(void *opaque)
{
    State *state = opaque;
    pthread_mutex_lock(&state->lock);
    state->released++;
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Initialize one native fixture and drain its mandatory initial snapshot.
 * Создать native fixture и прочитать initial snapshot. */
static void init(State *state, unsigned long long blocked)
{
    *state = (State){0};
    check(!pthread_mutex_init(&state->lock, NULL) && !pthread_cond_init(&state->gate, NULL), "fixture locks");
    state->block = blocked;
    state->source = dvm_changes_create();
    StyxEventPolicy policy = {DVM_CHANGE_CONTENT|DVM_CHANGE_RIGHTS, 1, sizeof(StyxEvent), allow, encode, state, release};
    state->events = styxeventsource(state->source, &policy);
    StyxEvent event;
    size_t count;
    check(state->events && styxeventsread(state->events, &event, sizeof(event), &count) == STYX_EVENT_RECORD, "initial snapshot");
}


/* EN/RU: Cleanup after every worker and adopted policy context retired.
 * Очистить после retirement всех worker и adopted policy context. */
static void done(State *state)
{
    check(state->released == 1, "policy final release");
    dvm_changes_destroy(state->source);
    pthread_cond_destroy(&state->gate);
    pthread_mutex_destroy(&state->lock);
}


/* EN/RU: Publish one generation with the existing independent native journal lifetime pin.
 * Опубликовать generation под independent native journal pin. */
static void publish(State *state, unsigned long long generation)
{
    DVM_ChangeEvent event = {1, generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 42};
    check(dvm_changes_notify(state->source, &event) == 1, "native publication");
}


/* EN/RU: Publish generation2 on a separate native producer.
 * Опубликовать generation2 на отдельном native producer. */
static void *publishing(void *opaque)
{
    publish(opaque, 2);
    return NULL;
}


/* EN/RU: Wait until producer actually enters controlled filter, without timing guesses.
 * Дождаться actual producer entry без timing guesses. */
static void entered(State *state)
{
    pthread_mutex_lock(&state->lock);
    while (!state->entered)
    {
        pthread_cond_wait(&state->gate, &state->lock);
    }
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Unblock native fixture callback after teardown/reordering was exercised.
 * Разблокировать fixture callback после teardown/reordering. */
static void proceed(State *state)
{
    pthread_mutex_lock(&state->lock);
    state->proceed = 1;
    pthread_cond_broadcast(&state->gate);
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Native readers compete for one shared inbox, each record consumed exactly once.
 * Native readers разделяют inbox; каждая запись поглощается ровно один раз. */
static void *reading(void *opaque)
{
    State *state = opaque;
    StyxEvent event;
    size_t count;
    int result;
    while ((result = styxeventsread(state->events, &event, sizeof(event), &count)) == STYX_EVENT_RECORD)
    {
        check(event.generation >= 2 && event.generation <= 21, "reader generation bounds");
        unsigned long long bit = 1ULL << event.generation;
        pthread_mutex_lock(&state->lock);
        check(!(state->seen & bit), "duplicate shared record");
        state->seen |= bit;
        state->total++;
        pthread_mutex_unlock(&state->lock);
    }
    check(result == STYX_EVENT_EMPTY, "live shared source did not return AGAIN");
    return NULL;
}


/* EN/RU: Verify nonwaiting final-owner release, stale callback fencing and concurrent alias reads.
 * Проверить nonwaiting final-owner release, stale callback fencing и concurrent alias reads. */
int main(void)
{
    State state;
    pthread_t worker, other;
    init(&state, 2);
    check(!pthread_create(&worker, NULL, publishing, &state), "publisher create");
    entered(&state);
    styxeventsfree(state.events);
    check(!state.released, "state freed before admitted callback returned");
    proceed(&state);
    check(!pthread_join(worker, NULL), "publisher join");
    done(&state);
    init(&state, 2);
    check(!pthread_create(&worker, NULL, publishing, &state), "reordered publisher");
    entered(&state);
    publish(&state, 3);
    StyxEvent event;
    size_t count;
    check(styxeventsread(state.events, &event, sizeof(event), &count) == STYX_EVENT_RECORD && event.generation == 3 &&
          event.kinds == (DVM_CHANGE_CONTENT|DVM_CHANGE_RIGHTS) && !event.flags, "reordered gap scope");
    proceed(&state);
    check(!pthread_join(worker, NULL), "reordered join");
    check(styxeventsread(state.events, &event, sizeof(event), &count) == STYX_EVENT_EMPTY, "stale callback queued");
    styxeventsfree(state.events);
    done(&state);
    init(&state, 0);
    for (unsigned long long generation = 2; generation <= 21; generation++)
    {
        publish(&state, generation);
    }
    check(!pthread_create(&worker, NULL, reading, &state) && !pthread_create(&other, NULL, reading, &state), "reader threads");
    check(!pthread_join(worker, NULL) && !pthread_join(other, NULL) && state.total == 20, "shared readers lost records");
    styxeventsfree(state.events);
    done(&state);
    puts("PASS native event resource teardown/reorder/readers");
    return 0;
}
