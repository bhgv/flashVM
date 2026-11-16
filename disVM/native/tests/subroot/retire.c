/* Copyright (c) 2026 bhgv
 * EN: Native root retirement control returns before a deliberately held cleanup RPC completes.
 * RU: Native root retirement control возвращается до завершения намеренно held cleanup RPC.
 */
#define _POSIX_C_SOURCE 200809L
#include "../../vfs/mount_internal.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/* EN/RU: External fixture state remains alive until mocked native clunk returns.
 * External fixture state живёт до возврата mocked native clunk. */
typedef struct State
{
    pthread_mutex_t lock; /* Native fixture gate. */
    pthread_cond_t gate; /* Expose nonwaiting retirement without timing guesses. */
    int entered; /* Worker reached its actual clunk call. */
    int proceed; /* Permit the native cleanup operation to complete. */
    int finished; /* Mocked clunk returned ownership. */
} State;


/* EN/RU: Always-active fixture contract with explicit diagnostic.
 * Постоянный fixture contract с явной диагностикой. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "root retirement: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Mock only the protocol cleanup boundary; real rootlife worker/control remain unchanged.
 * Mock только protocol cleanup boundary; rootlife worker/control остаются настоящими. */
void styxclientclunk(StyxFid *root)
{
    State *state = (State *)root;
    pthread_mutex_lock(&state->lock);
    state->entered = 1;
    pthread_cond_broadcast(&state->gate);
    while (!state->proceed)
    {
        pthread_cond_wait(&state->gate, &state->lock);
    }
    state->finished = 1;
    pthread_cond_broadcast(&state->gate);
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: Wait for a concrete native boundary with a five-second fixture deadline.
 * Дождаться конкретной native boundary с пятсекундным fixture deadline. */
static void await(State *state, int *flag)
{
    struct timespec until;
    check(!clock_gettime(CLOCK_REALTIME, &until), "clock");
    until.tv_sec += 5;
    pthread_mutex_lock(&state->lock);
    while (!*flag)
    {
        check(!pthread_cond_timedwait(&state->gate, &state->lock, &until), "worker boundary deadline");
    }
    pthread_mutex_unlock(&state->lock);
}


/* EN/RU: The caller keeps progressing while only the independent native cleanup worker waits.
 * Caller продолжает работу, пока ждёт только independent native cleanup worker. */
int main(void)
{
    State state = {0};
    check(!pthread_mutex_init(&state.lock, NULL) && !pthread_cond_init(&state.gate, NULL), "fixture locks");
    StyxRootLife *life = styxmountrootlife((StyxFid *)&state);
    check(life != NULL, "native cleanup worker");
    styxmountrootretire(life);
    await(&state, &state.entered);
    pthread_mutex_lock(&state.lock);
    check(!state.finished, "retirement waited for held cleanup");
    state.proceed = 1;
    pthread_cond_broadcast(&state.gate);
    pthread_mutex_unlock(&state.lock);
    await(&state, &state.finished);
    pthread_cond_destroy(&state.gate);
    pthread_mutex_destroy(&state.lock);
    puts("PASS nonwaiting captured root retirement/native-only cleanup");
    return 0;
}
