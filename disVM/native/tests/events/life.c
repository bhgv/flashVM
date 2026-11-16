/* Copyright (c) 2026 bhgv
 * EN: Deterministic terminal-source ownership, replay, limits and detached delivery tests.
 * RU: Проверки ownership, replay, limits и detached delivery terminal-source.
 */
#include "life.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Fixture state outlives its callback and release; lock gates only test threads.
 * Fixture state живёт дольше callback/release; lock управляет только test threads. */
typedef struct State
{
    pthread_mutex_t lock; /* Protect counters and gate. */
    pthread_cond_t gate; /* Main/emitter handshake. */
    unsigned calls; /* Callback count. */
    unsigned releases; /* Exactly-once release oracle. */
    int reason; /* First terminal reason. */
    int block; /* Deliberately hold one test callback in flight. */
    int entered; /* Emitter admitted. */
    StyxWatch *self; /* Optional consumed subscription. */
} State;


/* EN/RU: Always-active check; valid is evaluated before a failure exits.
 * Постоянная проверка; valid вычисляется до failure exit. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal source: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Initialize caller-owned native fixture, no callback owns its storage.
 * Создать caller-owned fixture, callback не владеет её storage. */
static void init(State *state)
{
    *state = (State){0};
    check(!pthread_mutex_init(&state->lock, NULL), "mutex");
    check(!pthread_cond_init(&state->gate, NULL), "condition");
}


/* EN/RU: Verify exactly one state release; all worker threads have already joined.
 * Проверить один release state после join всех worker threads. */
static void done(State *state)
{
    check(state->releases == 1, "release count");
    pthread_cond_destroy(&state->gate);
    pthread_mutex_destroy(&state->lock);
}


/* EN/RU: Count release under fixture lock; never free externally owned test storage.
 * Считать release под fixture lock; external test storage не освобождать. */
static void release(void *opaque)
{
    State *state = opaque;
    pthread_mutex_lock(&state->lock);
    state->releases++;
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Copy terminal reason; self-unsubscribe and test-only gate expose lifetime races.
 * Копировать reason; self-unsubscribe и test-only gate раскрывают lifetime races. */
static void receive(void *opaque, int reason)
{
    State *state = opaque;
    if (state->self)
    {
        styxclientunwatch(state->self);
        state->self = NULL;
    }
    pthread_mutex_lock(&state->lock);
    state->calls++;
    state->reason = reason;
    state->entered = 1;
    pthread_cond_broadcast(&state->gate);
    while (state->block)
    {
        pthread_cond_wait(&state->gate, &state->lock);
    }
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Publish on an independent native worker; source owner pins entry.
 * Публиковать на отдельном native worker; source owner удерживает entry. */
static void *emit(void *opaque)
{
    styxlifeemit(opaque, STYX_LINK_FAILED);
    return NULL;
}


/* EN/RU: First reason is sticky, replay occurs once, detach/self-detach release exactly once.
 * Первый reason sticky, replay один раз, detach/self-detach освобождают один раз. */
static void replay(void)
{
    State first, late;
    init(&first);
    init(&late);
    StyxLife *life = styxlifecreate();
    check(life != NULL, "source allocation");
    first.self = styxlifewatch(life, receive, &first, release);
    check(first.self != NULL, "self watch");
    styxlifeemit(life, STYX_LINK_FAILED);
    styxlifeemit(life, STYX_LINK_CLOSED);
    StyxWatch *watch = styxlifewatch(life, receive, &late, release);
    check(watch && first.calls == 1 && late.calls == 1 && late.reason == STYX_LINK_FAILED, "sticky replay");
    styxlifequiet(life);
    check(!styxlifewatch(life, receive, &late, release), "quiet subscription accepted");
    styxclientunwatch(watch);
    styxlifedrop(life);
    done(&first);
    done(&late);
}


/* EN/RU: Bounded live nodes reject without adopting state; quiet emits no fake failure.
 * Bounded live nodes отказывают без принятия state; quiet не создаёт fake failure. */
static void limits(void)
{
    State states[257];
    StyxWatch *watches[256];
    StyxLife *life = styxlifecreate();
    check(life != NULL, "bounded source");
    for (unsigned i = 0; i < 257; i++)
    {
        init(&states[i]);
        StyxWatch *watch = styxlifewatch(life, receive, &states[i], release);
        check((watch != NULL) == (i < 256), "watcher limit");
        if (i < 256)
        {
            watches[i] = watch;
        }
    }
    styxlifequiet(life);
    styxlifeemit(life, STYX_LINK_FAILED);
    styxlifedrop(life);
    for (unsigned i = 0; i < 256; i++)
    {
        styxclientunwatch(watches[i]);
        check(!states[i].calls, "quiet callback admitted");
        done(&states[i]);
    }
    check(!states[256].releases, "rejected state adopted");
    release(&states[256]);
    done(&states[256]);
}


/* EN/RU: Detached publication pins links/state after unsubscribe, quiet and owner drop.
 * Detached publication удерживает links/state после unsubscribe, quiet и owner drop. */
static void inflight(void)
{
    State held, pending;
    init(&held);
    init(&pending);
    held.block = 1;
    StyxLife *life = styxlifecreate();
    check(life != NULL, "inflight source");
    StyxWatch *next = styxlifewatch(life, receive, &pending, release);
    StyxWatch *first = styxlifewatch(life, receive, &held, release);
    check(first && next, "inflight watches");
    pthread_t worker;
    check(!pthread_create(&worker, NULL, emit, life), "emit thread");
    pthread_mutex_lock(&held.lock);
    while (!held.entered)
    {
        pthread_cond_wait(&held.gate, &held.lock);
    }
    pthread_mutex_unlock(&held.lock);
    styxclientunwatch(first);
    styxclientunwatch(next);
    styxlifequiet(life);
    styxlifedrop(life);
    pthread_mutex_lock(&held.lock);
    check(!held.releases, "inflight state released early");
    held.block = 0;
    pthread_cond_broadcast(&held.gate);
    pthread_mutex_unlock(&held.lock);
    check(!pthread_join(worker, NULL), "emit join");
    check(held.calls == 1 && !pending.calls, "detached callback admission");
    done(&held);
    done(&pending);
}


/* EN/RU: Execute deterministic native-only ownership fixtures; no network or guest waits.
 * Запустить native-only ownership fixtures; без network/guest waits. */
int main(void)
{
    replay();
    limits();
    inflight();
    puts("PASS native terminal source lifetime");
    return 0;
}
