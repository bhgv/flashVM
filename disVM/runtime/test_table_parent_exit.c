/* Copyright (c) 2026 bhgv
 * EN: Hold a real table cancellation before its reply until the lease owner retires.
 * RU: Удерживать настоящую отмену таблицы до ответа и завершения владельца lease.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "runqueue.h"
#include "owner_messages.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t reply_gate = PTHREAD_MUTEX_INITIALIZER; /* Test handshake only. */
static pthread_cond_t ready = PTHREAD_COND_INITIALIZER; /* Predicate notification. */
static int held; /* Cancellation callback entered, under reply_gate. */
static int released; /* Parent retirement and successor completion observed, under reply_gate. */

int __real_dvm_bigtable_cancel_wait(Prog *);
Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), int);


/* EN: Require condition or fail with borrowed message; never read managed state.
 * RU: Проверить condition либо завершить тест с заимствованным message;
 * управляемая память не читается. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL table parent exit: %s\n", message);
        exit(10);
    }
}


/* EN: Suspend only native test coordination outside GC participation.
 * Return after predicate is set under reply_gate; no managed pointer is dereferenced.
 * RU: Ждать нативный предикат вне участия в GC. Вернуться после установки
 * predicate под reply_gate; не разыменовывать управляемые указатели при ожидании. */
static void wait_predicate(int *predicate)
{
    int state = dvm_runtime_gc_native_try_leave();
    require(state > 0, "test waiter leaves GC participation");
    pthread_mutex_lock(&reply_gate);
    while (!*predicate)
    {
        pthread_cond_wait(&ready, &reply_gate);
    }
    pthread_mutex_unlock(&reply_gate);
    dvm_runtime_gc_native_enter(state);
}


/* EN: Complete actual table FIFO cancellation, then delay before main.c replies.
 * Return the genuine successful result, or terminate the fixture on failure.
 * child is borrowed from its own resident. No scheduler mutex is held after the
 * real call returns; the frame and completion claim retain all production roots.
 * main.c's internal reply call cannot be intercepted by GNU --wrap from that TU.
 * RU: Выполнить настоящую отмену FIFO и задержаться до ответа из main.c.
 * Вернуть настоящий успешный результат либо завершить тест при ошибке.
 * child заимствован у своего потока. После вызова mutex планировщика не удержан;
 * кадр и completion claim сохраняют корни. Внутренний вызов ответа в main.c
 * не перехватывается GNU --wrap из той же единицы трансляции. */
int __wrap_dvm_bigtable_cancel_wait(Prog *child)
{
    int result = __real_dvm_bigtable_cancel_wait(child);
    require(result == 1, "genuine table cancellation removed the child's FIFO record");
    pthread_mutex_lock(&reply_gate);
    require(!held, "exactly one table cancellation reply is held");
    held = 1;
    pthread_cond_broadcast(&ready);
    pthread_mutex_unlock(&reply_gate);
    wait_predicate(&released);
    puts("PASS table cancellation resumed after lease parent retirement");
    return result;
}


/* EN: Capacities 101/102 are private handshake markers in the Limbo fixture.
 * Preserve the real constructor and all borrowed operands after synchronization.
 * RU: Вместимости 101/102 — метки синхронизации частного Limbo-теста.
 * После синхронизации вызвать настоящий конструктор с исходными аргументами. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity,
    void (*mover)(void), int guarded)
{
    if (capacity == 101)
    {
        wait_predicate(&held);
    }
    else if (capacity == 102)
    {
        pthread_mutex_lock(&reply_gate);
        require(held && !released, "release follows parent retirement");
        released = 1;
        pthread_cond_broadcast(&ready);
        pthread_mutex_unlock(&reply_gate);
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, guarded);
}
