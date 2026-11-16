/* Copyright (c) 2026 bhgv
 * EN: Native subscription admission races, quota retirement and shared-policy callback lifetime.
 * RU: Гонки native subscription admission, quota retirement и lifetime callbacks общей policy.
 */
#define _POSIX_C_SOURCE 200809L
#include "subscriptions.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define WORKERS 16 /* Simultaneous native admission attempts against a four-slot factory. */


/* EN/RU: External fixture storage survives all native workers and final policy release.
 * External fixture storage живёт дольше всех native workers и final policy release. */
typedef struct State
{
    pthread_mutex_t lock; /* Fixture gates/counters only, never production locks. */
    pthread_cond_t gate; /* Controlled native callback/admission barrier. */
    unsigned long long blocked; /* Source generation held in native allow callback. */
    int entered; /* Callback reached test-only barrier. */
    int proceed; /* Permit blocked callback or admission workers to retire. */
    int arrived; /* Admission workers reached their post-attempt barrier. */
    unsigned released; /* Exactly-once shared policy release. */
    DVM_Changes *journal; /* Owned main fixture pin through all publisher joins. */
} State;


/* EN/RU: Keep fixture checks enabled in all compiler configurations.
 * Сохранить fixture checks во всех compiler configurations. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "subscription race: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Deliberately hold a native test callback to expose reference/quota retirement.
 * Намеренно удержать native test callback для проверки reference/quota retirement. */
static int allow(void *opaque, const StyxEvent *event)
{
    State *state = opaque;
    pthread_mutex_lock(&state->lock);
    if (event->generation == state->blocked)
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


/* EN/RU: Copy fixture scalar record; no policy state mutation during concurrent readers.
 * Скопировать fixture scalar record; без изменения policy state при concurrent readers. */
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


/* EN/RU: Record final shared-policy retirement under external fixture lock.
 * Записать final shared-policy retirement под external fixture lock. */
static void release(void *opaque)
{
    State *state = opaque;
    pthread_mutex_lock(&state->lock);
    state->released++;
    pthread_cond_broadcast(&state->gate);
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Wait for a concrete fixture counter with a bounded native deadline.
 * Дождаться конкретного fixture counter с bounded native deadline. */
static void waitfor(State *state, int *counter, int expected)
{
    struct timespec until;
    check(!clock_gettime(CLOCK_REALTIME, &until), "clock");
    until.tv_sec += 5;
    pthread_mutex_lock(&state->lock);
    while (*counter != expected)
    {
        check(!pthread_cond_timedwait(&state->gate, &state->lock, &until), "native gate deadline");
    }
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Release controlled native callbacks/workers without invoking any guest.
 * Освободить controlled native callbacks/workers без вызова guest. */
static void proceed(State *state)
{
    pthread_mutex_lock(&state->lock);
    state->proceed = 1;
    pthread_cond_broadcast(&state->gate);
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Each native admission worker owns a factory capability and publishes its result once.
 * Каждый native admission worker владеет factory capability и публикует result один раз. */
typedef struct Work
{
    State *state; /* Borrowed external state through worker join. */
    StyxSubscriptions *factory; /* Owned worker capability through subscribe return. */
    StyxEvents *events; /* Owned result; main observes only after gate/join. */
    int hold; /* Retain result until all concurrent attempts are observed. */
} Work;


/* EN/RU: Subscribe on a real native thread; optional barrier holds successful quota slots.
 * Подписаться на настоящем native thread; optional barrier удерживает quota slots. */
static void *subscribe(void *opaque)
{
    Work *work = opaque;
    work->events = styxsubscribe(work->factory);
    if (work->hold)
    {
        pthread_mutex_lock(&work->state->lock);
        work->state->arrived++;
        pthread_cond_broadcast(&work->state->gate);
        while (!work->state->proceed)
        {
            pthread_cond_wait(&work->state->gate, &work->state->lock);
        }
        pthread_mutex_unlock(&work->state->lock);
        styxeventsfree(work->events);
    }
    styxsubscriptionsfree(work->factory);
    return NULL;
}


/* EN/RU: Publish one actual journal notification from an independent native thread.
 * Опубликовать actual journal notification с независимого native thread. */
static void *publish(void *opaque)
{
    State *state = opaque;
    DVM_ChangeEvent event = {1, 2, STYX_EVENT_CONTENT, 0, 0};
    check(dvm_changes_notify(state->journal, &event) == 1, "publication");
    return NULL;
}


/* EN/RU: Race explicit admission close with an already reserved subscriber's initial callback.
 * Гонка explicit admission close с initial callback уже reserved subscriber. */
static void setup(State *state, StyxSubscriptions *factory)
{
    Work work = {state, factory, NULL, 0};
    pthread_t thread;
    check(styxsubscriptionsretain(factory) && !pthread_create(&thread, NULL, subscribe, &work), "setup worker");
    waitfor(state, &state->entered, 1);
    StyxSubscriptionInfo info;
    check(styxsubscriptionsinfo(factory, &info) && info.active == 1, "setup did not reserve quota");
    styxsubscriptionsclose(factory);
    check(!styxsubscribe(factory), "close admitted another setup");
    styxsubscriptionsfree(factory);
    check(!state->released, "close waited/freed in-flight policy");
    proceed(state);
    check(!pthread_join(thread, NULL) && work.events && !state->released, "admitted setup lost lifetime");
    styxeventsfree(work.events);
    check(state->released == 1, "setup final cleanup");
}


/* EN/RU: Final source owner may detach while publisher holds policy; quota remains occupied.
 * Последний source owner может detach при held publisher; quota остаётся занятой. */
static void retirement(State *state, StyxSubscriptions *factory)
{
    StyxEvents *events = styxsubscribe(factory);
    check(events != NULL, "initial subscription");
    pthread_t thread;
    check(!pthread_create(&thread, NULL, publish, state), "publisher thread");
    waitfor(state, &state->entered, 1);
    styxeventsfree(events);
    StyxSubscriptionInfo info;
    check(styxsubscriptionsinfo(factory, &info) && info.active == 1 && !styxsubscribe(factory), "in-flight callback released quota early");
    styxsubscriptionsfree(factory);
    check(!state->released, "in-flight callback lost shared policy");
    proceed(state);
    check(!pthread_join(thread, NULL) && state->released == 1, "callback final cleanup");
}


/* EN/RU: Sixteen simultaneous native callers must admit exactly four bounded independent sources.
 * Шестнадцать simultaneous native callers допускают ровно четыре bounded independent sources. */
static void quota(State *state, StyxSubscriptions *factory)
{
    Work work[WORKERS];
    pthread_t threads[WORKERS];
    for (unsigned i = 0; i < WORKERS; i++)
    {
        work[i] = (Work){state, factory, NULL, 1};
        check(styxsubscriptionsretain(factory) && !pthread_create(&threads[i], NULL, subscribe, &work[i]), "quota worker");
    }
    waitfor(state, &state->arrived, WORKERS);
    unsigned accepted = 0;
    for (unsigned i = 0; i < WORKERS; i++)
    {
        accepted += work[i].events != NULL;
    }
    StyxSubscriptionInfo info;
    check(accepted == 4 && styxsubscriptionsinfo(factory, &info) && info.active == 4, "racing admission exceeded quota");
    proceed(state);
    for (unsigned i = 0; i < WORKERS; i++)
    {
        check(!pthread_join(threads[i], NULL), "quota join");
    }
    check(styxsubscriptionsinfo(factory, &info) && !info.active && !state->released, "retired quota not reusable");
    styxsubscriptionsfree(factory);
    check(state->released == 1, "quota policy cleanup");
}


/* EN/RU: Run deterministic native barriers against real factory/source lifetime contracts.
 * Выполнить deterministic native barriers на настоящих factory/source lifetime contracts. */
int main(int argc, char **argv)
{
    check(argc == 2, "scenario");
    State state = {0};
    check(!pthread_mutex_init(&state.lock, NULL) && !pthread_cond_init(&state.gate, NULL), "fixture locks");
    state.blocked = !strcmp(argv[1], "setup") ? 1 : !strcmp(argv[1], "retirement") ? 2 : 0;
    state.journal = dvm_changes_create();
    StyxEventPolicy policy = {STYX_EVENT_CONTENT, 0, sizeof(StyxEvent), allow, encode, &state, release};
    StyxSubscriptions *factory = styxsubscriptions(state.journal, &policy, !strcmp(argv[1], "quota") ? 4 : 1);
    check(factory != NULL, "factory");
    if (!strcmp(argv[1], "setup"))
    {
        setup(&state, factory);
    }
    else if (!strcmp(argv[1], "retirement"))
    {
        retirement(&state, factory);
    }
    else
    {
        check(!strcmp(argv[1], "quota"), "unknown scenario");
        quota(&state, factory);
    }
    dvm_changes_destroy(state.journal);
    pthread_cond_destroy(&state.gate);
    pthread_mutex_destroy(&state.lock);
    printf("PASS independent subscription race: %s\n", argv[1]);
    return 0;
}
