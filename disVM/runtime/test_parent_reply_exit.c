/* Copyright (c) 2026 bhgv
 * EN: Hold a real child cancellation callback until its parent has retired.
 * RU: Удерживать настоящий callback отмены ребёнка до завершения родителя.
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
static int released; /* Grandparent's killwait completed, under reply_gate. */

void __real_dvm_runtime_wait_request_finished(Prog *, DVM_OwnerMessage *);
Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), int);


/* EN: Fail the private fixture on protocol errors; no managed state is read.
 * RU: Завершить тест при нарушении протокола, не читая управляемую память. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL parent reply exit: %s\n", message);
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


/* EN: Delay the borrowed child/message before real reply lookup. They remain
 * owned by the live child and its completion claim while parent is destroyed.
 * RU: Задержать заимствованные child/message до поиска адресата ответа.
 * Они принадлежат живому ребёнку и его completion claim при удалении родителя. */
void __wrap_dvm_runtime_wait_request_finished(Prog *child, DVM_OwnerMessage *message)
{
    if (message->kind == DVM_PROG_MESSAGE_CANCEL_WAIT)
    {
        pthread_mutex_lock(&reply_gate);
        require(!held, "only one cancellation reply is held");
        held = 1;
        pthread_cond_broadcast(&ready);
        pthread_mutex_unlock(&reply_gate);
        wait_predicate(&released);
        puts("PASS cancellation callback resumed after parent retirement");
    }
    __real_dvm_runtime_wait_request_finished(child, message);
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
