/* Copyright (c) 2026 bhgv
 * EN: Native cancellation replay, TLS isolation and detached callback/source lifetimes.
 * RU: Native cancellation replay, TLS isolation и detached callback/source lifetimes.
 */
#include "cancel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: External native fixture state survives all producer/callback joins.
 * External native fixture state живёт до всех producer/callback joins. */
typedef struct State
{
    pthread_mutex_t lock; /* Fixture-only native callback boundary. */
    pthread_cond_t changed; /* Controlled in-flight teardown, no guest/thread stopping. */
    DVM_Cancel *cancel; /* Producer-owned independent pin. */
    unsigned received; /* Protected signal count, duplicates permitted. */
    unsigned released; /* Exact final callback retirement. */
    int hold; /* Artificially hold a callback only in the lifetime probe. */
    int proceed; /* Permit diagnostic callback to return. */
} State;


/* EN/RU: Always-active semantic contracts, no build-mode assertions.
 * Постоянные semantic contracts, без build-mode assertions. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native cancel scope: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Copy a native signal; a deliberate test gate exposes already-admitted callback teardown.
 * Скопировать native signal; test gate показывает teardown already-admitted callback.
 * Production callbacks do not wait. Production callbacks не ожидают. */
static void receive(void *opaque)
{
    State *state = opaque;
    pthread_mutex_lock(&state->lock);
    state->received++;
    pthread_cond_broadcast(&state->changed);
    while (state->hold && !state->proceed)
    {
        pthread_cond_wait(&state->changed, &state->lock);
    }
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Count exact final callback storage retirement without freeing external fixture state.
 * Посчитать exact final callback retirement, external fixture state не освобождается. */
static void release(void *opaque)
{
    State *state = opaque;
    pthread_mutex_lock(&state->lock);
    state->released++;
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Independent OS thread inherits no source TLS and owns its source pin through request.
 * Independent OS thread не наследует TLS и владеет source pin на время request. */
static void *producer(void *opaque)
{
    State *state = opaque;
    check(dvm_cancel_current() == NULL, "native TLS inherited by a new thread");
    dvm_cancel_request(state->cancel);
    dvm_cancel_release(state->cancel);
    return NULL;
}


/* EN/RU: Allocate external native fixture synchronization; caller later joins/destroys it.
 * Создать external fixture synchronization; caller затем joins/destroys. */
static void initialize(State *state)
{
    check(!pthread_mutex_init(&state->lock, NULL) && !pthread_cond_init(&state->changed, NULL), "fixture synchronization");
    state->cancel = dvm_cancel_create();
    check(state->cancel != NULL, "native source");
}


/* EN/RU: Destroy native fixture after all actual callback/producer owners retired.
 * Освободить native fixture после retirement всех callback/producer owners. */
static void finish(State *state)
{
    pthread_cond_destroy(&state->changed);
    pthread_mutex_destroy(&state->lock);
}


/* EN/RU: Concurrent subscription/request never loses sticky cancellation or double-releases state.
 * Concurrent subscribe/request не теряет sticky cancellation и не double-release state. */
static void replay(void)
{
    for (unsigned i = 0; i < 64; i++)
    {
        State state = {0};
        initialize(&state);
        check(dvm_cancel_retain(state.cancel), "producer pin");
        pthread_t worker;
        check(!pthread_create(&worker, NULL, producer, &state), "native producer");
        DVM_CancelTap *tap = dvm_cancel_subscribe(state.cancel, receive, &state, release);
        check(tap != NULL && !pthread_join(worker, NULL), "subscription/producer completion");
        dvm_cancel_request(state.cancel);
        dvm_cancel_unsubscribe(tap);
        check(state.received >= 1 && state.received <= 2 && state.released == 1, "sticky replay/lifetime");
        dvm_cancel_release(state.cancel);
        finish(&state);
    }
}


/* EN/RU: Scope LIFO and source lifetime remain valid through concurrent native publication.
 * Scope LIFO/source lifetime сохраняются при concurrent native publication. */
int main(void)
{
    replay();
    State state = {0};
    initialize(&state);
    state.hold = 1;
    DVM_CancelScope outer = {0}, inner = {0};
    DVM_Cancel *other = dvm_cancel_create();
    check(other && dvm_cancel_enter(&outer, state.cancel) && dvm_cancel_enter(&inner, other), "nested scopes");
    check(!dvm_cancel_leave(&outer) && dvm_cancel_current() == other, "out-of-order leave changed scope");
    check(dvm_cancel_leave(&inner) && dvm_cancel_current() == state.cancel, "restore enclosing scope");
    dvm_cancel_release(other);
    DVM_CancelTap *tap = dvm_cancel_subscribe(state.cancel, receive, &state, release);
    check(tap && dvm_cancel_retain(state.cancel), "in-flight source pin");
    pthread_t worker;
    check(!pthread_create(&worker, NULL, producer, &state), "in-flight producer");
    pthread_mutex_lock(&state.lock);
    while (!state.received)
    {
        pthread_cond_wait(&state.changed, &state.lock);
    }
    pthread_mutex_unlock(&state.lock);
    dvm_cancel_unsubscribe(tap);
    check(dvm_cancel_leave(&outer) && dvm_cancel_current() == NULL, "scope cleanup waited/failed");
    dvm_cancel_release(state.cancel);
    pthread_mutex_lock(&state.lock);
    check(!state.released, "in-flight callback storage retired early");
    state.proceed = 1;
    pthread_cond_broadcast(&state.changed);
    pthread_mutex_unlock(&state.lock);
    check(!pthread_join(worker, NULL) && state.released == 1, "detached callback/source lifetime");
    finish(&state);
    puts("PASS native cancel scopes replay TLS LIFO and in-flight teardown");
    return 0;
}
