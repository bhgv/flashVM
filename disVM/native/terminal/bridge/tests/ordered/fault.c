/* Copyright (c) 2026 bhgv
 * EN: Native-only setup refusal, delayed-callback lifetime tests and held dispatcher checkpoints.
 * RU: Native-отказы setup, проверки lifetime поздних callbacks и checkpoints удержанного dispatcher.
 */
#define _POSIX_C_SOURCE 200809L
#include "bridge/ordered.h"
#include "cancel.h"
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static unsigned calls; /* Count native control construction only in invalid-options test. */


/* EN/RU: Fail a fixture contract explicitly; no guest/runtime operation.
 * Явно завершить fixture при нарушении контракта; без гостевых/runtime-операций.
 */
static void check(int valid)
{
    if (!valid)
    {
        fprintf(stderr, "ordered terminal fault case %d\n", ORDERED_TERMINAL_CASE);
        exit(2);
    }
}

#if ORDERED_TERMINAL_CASE == 6


/* EN/RU: Original captured constructor; caller retains original client/descriptor authority.
 * Исходный конструктор; caller сохраняет исходные полномочия client/descriptor.
 */
extern StyxBidirControl *__real_styxbidircontrol(StyxClient *, const DVM_TerminalChannels *, unsigned, unsigned);


/* EN/RU: Detect any unwanted network-capable construction after invalid native options.
 * Выявить нежелательное создание сетевого control при неверных native-options.
 */
StyxBidirControl *__wrap_styxbidircontrol(StyxClient *client, const DVM_TerminalChannels *channels, unsigned limit, unsigned slots)
{
    calls++;
    return __real_styxbidircontrol(client, channels, limit, slots);
}
#endif


/* EN/RU: Native fixture counter, called only after synchronous constructor attempts finish.
 * Native-счётчик fixture, читается после синхронных попыток создания.
 */
unsigned orderedterminalcalls(void)
{
    return calls;
}

#if ORDERED_TERMINAL_CASE == 7


/* EN/RU: Refuse notification setup without adopting state; constructor must perform its normal rollback.
 * Отказать подписке без принятия state; конструктор должен выполнить обычный rollback.
 */
DVM_CancelTap *__wrap_styxorderedwatch(StyxOrdered *peer, void (*receive)(void *), void *state, void (*release)(void *))
{
    (void)peer;
    (void)receive;
    (void)state;
    (void)release;
    return NULL;
}
#endif

#if ORDERED_TERMINAL_CASE == 12


/* EN/RU: Refuse lower-control watch without adopting state; broker rollback must release its partial owner.
 * Отказать нижней подписке без принятия state; rollback брокера освобождает частично созданный owner.
 */
DVM_CancelTap *__wrap_styxbidircontrolwatch(StyxBidirControl *control, void (*receive)(void *), void *state, void (*release)(void *))
{
    (void)control;
    (void)receive;
    (void)state;
    (void)release;
    return NULL;
}
#endif

#if ORDERED_TERMINAL_CASE == 13 || ORDERED_TERMINAL_CASE == 14
static int armed; /* Original server/client threads are established before arming this fixture. */


/* EN/RU: Original native thread creation; caller retains argument until join.
 * Исходное создание native-потока; caller удерживает argument до join.
 */
extern int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);


/* EN/RU: Refuse broker RX or bridge TX creation after prior native resources exist; never alter existing threads.
 * Отказать созданию RX брокера либо TX моста после подготовки ресурсов; существующие потоки не меняются.
 */
int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attributes, void *(*entry)(void *), void *argument)
{
    if (armed && ++calls == (ORDERED_TERMINAL_CASE == 13 ? 3U : 6U))
    {
        return EAGAIN; /* Fixture-only native creation failure, no thread adopted. */
    }
    return __real_pthread_create(thread, attributes, entry, argument);
}
#endif


/* EN/RU: Arm only post-fixture native construction, no mutation in ordinary acceptance cases.
 * Отмечать лишь native-создание после fixture, без изменений в обычных проверках.
 */
void orderedterminalarm(void)
{
#if ORDERED_TERMINAL_CASE == 13 || ORDERED_TERMINAL_CASE == 14
    armed = 1;
#endif
}

#if ORDERED_TERMINAL_CASE == 9 || ORDERED_TERMINAL_CASE == 10
static void (*saved_receive)(void *); /* Original callback retained beyond owner's disposal by fixture. */
static void (*saved_release)(void *); /* Original final state disposal. */
static void *saved_state; /* Fixture-retained independent lifetime fence, never owner itself. */
static int detached; /* Actual subscription final release occurred. */


/* EN/RU: Delay original callback while its independently owned fence stays alive.
 * Задержать исходный callback, сохраняя его независимо принадлежащий fence.
 */
static void delay(void *state)
{
    check(state == saved_state);
}


/* EN/RU: Transfer final context ownership to fixture until post-free callback/release, no lost allocation.
 * Передать последнее владение context fixture до callback/release после free, без потери выделения.
 */
static void retain(void *state)
{
    check(state == saved_state);
    detached = 1;
}

#if ORDERED_TERMINAL_CASE == 9


/* EN/RU: Actual source-pinning ordered broker subscription, state adopted only on success.
 * Настоящая подписка на source брокера; state принимается лишь при успехе.
 */
extern DVM_CancelTap *__real_styxorderedwatch(StyxOrdered *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Delay an actual UI-bridge retirement callback until its bridge has been freed.
 * Задержать настоящий callback UI-моста до освобождения его bridge.
 */
DVM_CancelTap *__wrap_styxorderedwatch(StyxOrdered *peer, void (*receive)(void *), void *state, void (*release)(void *))
{
    check(!saved_state);
    saved_receive = receive;
    saved_release = release;
    saved_state = state;
    return __real_styxorderedwatch(peer, delay, state, retain);
}
#else


/* EN/RU: Actual lower-control source subscription, state adopted only on success.
 * Настоящая подписка нижнего control; state принимается лишь при успехе.
 */
extern DVM_CancelTap *__real_styxbidircontrolwatch(StyxBidirControl *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Delay lower-control callback until ordered peer/engine/control have all retired.
 * Задержать callback нижнего control до завершения peer/engine/control.
 */
DVM_CancelTap *__wrap_styxbidircontrolwatch(StyxBidirControl *control, void (*receive)(void *), void *state, void (*release)(void *))
{
    check(!saved_state);
    saved_receive = receive;
    saved_release = release;
    saved_state = state;
    return __real_styxbidircontrolwatch(control, delay, state, retain);
}
#endif
#endif


/* EN/RU: Trigger fixture-owned late callbacks after production lifetime fences detached access, then release exactly once.
 * Вызвать поздние callbacks после отделения production-fences, затем освободить context ровно один раз.
 */
void orderedterminallate(void)
{
#if ORDERED_TERMINAL_CASE == 9 || ORDERED_TERMINAL_CASE == 10
    check(detached && saved_state && saved_receive && saved_release);
    saved_receive(saved_state);
    saved_release(saved_state);
    saved_state = NULL;
#else
    check(1);
#endif
}

#if ORDERED_TERMINAL_CASE == 11
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Native test-only dispatcher lifetime gate. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Exact worker-entry notifications. */
static int rx_entered; /* Native dispatcher exists but has processed no record. */
static int tx_entered; /* Real bridge TX copy is about to wait for profile activation. */
static int released; /* Main observed actual physical retirement and permits joined cleanup. */


/* EN/RU: Original native dispatcher, owner borrowed until join.
 * Исходный native-dispatcher, owner заимствован до join.
 */
extern void *__real_orderedreceive(void *);


/* EN/RU: Hold dispatcher before it can see an input/terminal record; physical watch must still close UI channels.
 * Удерживать dispatcher до input/terminal-записей; physical watch всё равно обязан закрыть UI-каналы.
 */
void *__wrap_orderedreceive(void *peer)
{
    pthread_mutex_lock(&gate);
    rx_entered = 1;
    pthread_cond_broadcast(&changed);
    while (!released)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    return __real_orderedreceive(peer);
}


/* EN/RU: Original native per-client send wait, owner/input borrowed through return.
 * Исходное native-ожидание отправки клиента, owner/input заимствованы до возврата.
 */
extern int __real_styxorderedclientsendwait(StyxOrderedClient *, const void *, size_t, unsigned);


/* EN/RU: Mark retained bridge output before invoking the unchanged real native wait.
 * Отметить сохранённый output моста до неизменённого настоящего native-ожидания.
 */
int __wrap_styxorderedclientsendwait(StyxOrderedClient *client, const void *bytes, size_t size, unsigned timeout)
{
    pthread_mutex_lock(&gate);
    tx_entered = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    return __real_styxorderedclientsendwait(client, bytes, size, timeout);
}
#endif


/* EN/RU: Wait for exact native entry conditions, no production/guest polling or timing guesses.
 * Ждать точные native-checkpoints, без production/guest polling и временных предположений.
 */
void orderedterminalblocked(void)
{
#if ORDERED_TERMINAL_CASE == 11
    struct timespec deadline; /* One five-second test gate budget. */
    check(!clock_gettime(CLOCK_REALTIME, &deadline));
    deadline.tv_sec += 5;
    pthread_mutex_lock(&gate);
    while (!rx_entered || !tx_entered)
    {
        check(!pthread_cond_timedwait(&changed, &gate, &deadline));
    }
    pthread_mutex_unlock(&gate);
#endif
}


/* EN/RU: Release test-only held dispatcher after real retirement observation, before join/free.
 * Отпустить test-only dispatcher после реального завершения, до join/free.
 */
void orderedterminalrelease(void)
{
#if ORDERED_TERMINAL_CASE == 11
    pthread_mutex_lock(&gate);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
#endif
}

#if ORDERED_TERMINAL_CASE != 5


/* EN/RU: Link-only stub, actual AS ownership test supplies its separate implementation in case5.
 * Только link-stub; настоящий AS-тест владения предоставляет реализацию в case5.
 */
void orderedterminalas(DVM_UIBridge *local)
{
    (void)local;
    check(0);
}
#endif
