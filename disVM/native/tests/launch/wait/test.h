/* Copyright (c) 2026 bhgv
 * EN: Native-only deterministic admission wait gates; no timing sleeps or guest state.
 * RU: Native-only детерминированные admission wait gates, без timing sleeps и guest state.
 */
#ifndef LAUNCH_WAIT_TEST_H
#define LAUNCH_WAIT_TEST_H
#include "../backend.h"
#include "../../../../runtime/vfs/launch/service.h"
#include <pthread.h>


/* EN/RU: Test worker storage lives until join; native pointers remain test-private.
 * Test worker storage живёт до join; native pointers остаются test-private.
 */
typedef struct Waiter
{
    DVM_LaunchService *owner; /* Borrowed service through join. */
    DVM_Cancel *cancel; /* Optional borrowed independent per-operation source. */
    DVM_LaunchReview *review; /* Owned result after join, NULL on non-request outcome. */
    DVM_LaunchWaitResult result; /* Copied actual public API status. */
    pthread_t thread; /* Native worker joined exactly once. */
    unsigned waits; /* Count actual calls reaching native condition sleep; gate-locked. */
    int hold_empty; /* Immutable pre-start gate after first empty queue inspection. */
    int empty_seen; /* Gate-locked observation before the wait predicate. */
    int release_empty; /* Gate-locked release of empty inspection. */
    int hold_claim; /* Immutable pre-start gate after a successful queue claim. */
    int claim_seen; /* Gate-locked queue-claim observation. */
    int release_claim; /* Gate-locked release after main cancellation/close. */
    int done; /* Gate-locked completed public wait call. */
} Waiter;


/* EN/RU: Start one worker borrowing its zeroed/configured storage through join; fatal construction failure.
 * Запустить worker с zeroed/configured storage до join; construction failure fatal.
 */
void waiter_start(Waiter *waiter);


/* EN/RU: Await copied worker progress:kind0 native wait,1 empty gate,2 claim gate,3 done; no sleep/poll.
 * Дождаться worker progress:kind0 native wait,1 empty gate,2 claim gate,3 done; без sleep/poll.
 */
void waiter_await(Waiter *waiter, int kind);


/* EN/RU: Release one previously observed gate;kind1 empty/kind2 claim, no service/VM operations.
 * Освободить observed gate;kind1 empty/kind2 claim, без service/VM operations.
 */
void waiter_release(Waiter *waiter, int kind);


/* EN/RU: Join native worker before reading review/status or retiring its owner/source.
 * Завершить native worker до чтения review/status и освобождения owner/source.
 */
void waiter_join(Waiter *waiter);

/* EN/RU: Await any completed worker in a borrowed array; return its index, gate-locked scan without polling.
 * Дождаться любого completed worker в borrowed array; вернуть index, gate-locked scan без polling.
 */
unsigned waiter_any(Waiter *waiters, unsigned count);
#endif
