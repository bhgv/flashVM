/* Copyright (c) 2026 bhgv
 * EN: Deterministic clean-EOF barrier: hold the final accepted lower record until its retirement callback finishes.
 * RU: Детерминированная граница чистого EOF: удержать последнюю нижнюю запись до завершения retirement-callback.
 */
#define _POSIX_C_SOURCE 200809L
#include "order/broker/broker.h"
#include "flashvm/sordctl.h"
#include "cancel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Test-only ordered EOF checkpoints. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Native checkpoint condition. */
static int lower_ended; /* Original lower retirement callback completed. */
static int peer_ended; /* Broker itself completed accepted-record drain and ended. */
static void (*original_receive)(void *); /* Original native callback, not a replacement implementation. */
static void (*original_release)(void *); /* Original callback state ownership. */


/* EN/RU: Fatal explicit native checkpoint contract.
 * Явная обязательная проверка native-checkpoint.
 */
static void check(int valid)
{
    if (!valid)
    {
        fputs("CLI clean-EOF checkpoint failed\n", stderr);
        exit(3);
    }
}


/* EN/RU: Wait bounded native fixture time for exact flag, not a production delay or guest poll.
 * Ждать флаг с ограниченным native-временем fixture, не production-delay и не guest-poll.
 */
static void waitfor(int *flag)
{
    struct timespec deadline; /* One five-second fixture budget. */
    check(!clock_gettime(CLOCK_REALTIME, &deadline));
    deadline.tv_sec += 5;
    pthread_mutex_lock(&gate);
    while (!*flag)
    {
        check(!pthread_cond_timedwait(&changed, &gate, &deadline));
    }
    pthread_mutex_unlock(&gate);
}


/* EN/RU: Real lower-control subscription entry, state adopted only on success.
 * Настоящая подписка lower-control, state принимается лишь при успехе.
 */
extern DVM_CancelTap *__real_styxbidircontrolwatch(StyxBidirControl *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Invoke original callback first, then expose its actual completion to the held dispatcher.
 * Сперва вызвать исходный callback, затем отметить его действительное завершение для dispatcher.
 */
static void lower(void *state)
{
    original_receive(state);
    pthread_mutex_lock(&gate);
    lower_ended = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
}


/* EN/RU: Preserve actual callback final ownership, no fixture pin after release.
 * Сохранить настоящий final ownership callback, без fixture-pin после release.
 */
static void release(void *state)
{
    original_release(state);
}


/* EN/RU: Wrap one production subscription, preserving all real retirement/state work.
 * Обернуть production-подписку, сохранив всю реальную retirement/state-работу.
 */
DVM_CancelTap *__wrap_styxbidircontrolwatch(StyxBidirControl *control, void (*receive)(void *), void *state, void (*dispose)(void *))
{
    original_receive = receive;
    original_release = dispose;
    return __real_styxbidircontrolwatch(control, lower, state, release);
}


/* EN/RU: Real lower record receive, outputs retain their production borrowing/copy contract.
 * Настоящее получение нижней записи, outputs сохраняют production-контракт.
 */
extern int __real_styxbidircontrolnext(StyxBidirControl *, unsigned long *, void *, size_t, size_t *);


/* EN/RU: Hold exactly the final data record after lower queue pop but before ordered dispatch.
 * Удержать точно последнюю data-запись после lower-pop, но до ordered-dispatch.
 */
int __wrap_styxbidircontrolnext(StyxBidirControl *control, unsigned long *tag, void *bytes, size_t capacity, size_t *size)
{
    int result = __real_styxbidircontrolnext(control, tag, bytes, capacity, size); /* Actual original record. */
    FVM_SOrderedRecord record; /* Verification-only borrowed view. */
    size_t used; /* Exact packet extent. */
    if (result == STYX_CONTROL_RECORD && *tag == 91 &&
        fvm_sordered_read(bytes, *size, 256, &record, &used) == FVM_S_OK && record.record.size == 4 &&
        !memcmp(record.record.payload, "last", 4))
    {
        waitfor(&lower_ended);
    }
    return result;
}


/* EN/RU: Observe actual peer retirement without borrowing peer in callback state.
 * Наблюдать реальный peer-retirement без указателя на peer в callback-state.
 */
static void ended(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&gate);
    peer_ended = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
}


/* EN/RU: Real directional readiness wait, native peer remains owned by connection constructor.
 * Настоящее ожидание направления; native-peer принадлежит connection-конструктору.
 */
extern int __real_styxorderedwait(StyxOrdered *, unsigned, unsigned);


/* EN/RU: Delay startup observation until peer finished EOF; accepted-once history must still allow buffered RX drain.
 * Отложить startup-наблюдение до EOF peer; история согласования должна позволить drain готового RX.
 */
int __wrap_styxorderedwait(StyxOrdered *peer, unsigned direction, unsigned timeout)
{
    if (direction != STYX_ORDER_RX)
    {
        return __real_styxorderedwait(peer, direction, timeout);
    }
    DVM_CancelTap *tap = styxorderedwatch(peer, ended, NULL, NULL); /* Source pin survives the fixture checkpoint. */
    check(tap != NULL);
    int result = __real_styxorderedwait(peer, direction, timeout); /* Preserve exact public result. */
    waitfor(&peer_ended);
    dvm_cancel_unsubscribe(tap);
    return result;
}
