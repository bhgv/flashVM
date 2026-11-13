/* Copyright (c) 2026 bhgv
 * EN: Hold the first table cancellation until its parent posts a second operation.
 * RU: Задержать первую отмену таблицы до второго запроса того же родителя.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "runqueue.h"
#include "table_leases.h"
#include "managed_workers.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;  /* Test predicates only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Predicate notification. */
static void *nodes[8];                                    /* Unique owned cancellation allocations, under gate. */
static int allocations;        /* Cancellation allocations observed before send, under gate. */
static int disposals;          /* Observed nodes freed, under gate. */
static int held;               /* First child's real table cancellation is delayed, under gate. */
static int released;           /* Second request is published, under gate. */
static int first_applied;      /* Parent consumed A while waiting for B, under gate. */
static __thread int saw_reply; /* Owner-local to middle parent; a reply was dequeued. */
static int child_calls;        /* Child-owner-local table cancellation count. */
static int attempts;           /* Middle parent's cancellation calls, under gate. */


/* EN: Production linker targets, preserving borrowed arguments and exact results.
 * RU: Production-цели линкера с исходными заимствованными аргументами и результатами. */
DVM_OwnerMessage *__real_dvm_messages_take(DVM_MessageEndpoint *endpoint, int reply, int wait);
void __real_dvm_managed_workers_finish_reply(Prog *parent);
void __real_free(void *pointer);
int __real_dvm_messages_send(DVM_MessageRouter *, uintptr_t, int, DVM_OwnerMessage *);
int __real_dvm_bigtable_cancel_wait(Prog *child);
int __real_dvm_runtime_prepare_cancelwait(Prog *parent, int pid);
Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), int);


/* EN: Fail with borrowed diagnostic on broken test protocol; never returns on error.
 * RU: Завершить тест с заимствованной диагностикой при нарушении протокола. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL cancellation generation: %s\n", message);
        exit(10);
    }
}


/* EN: Wait for predicate outside GC admission. Keep this thread's Prog/REG;
 * restore admission before returning, without managed access during the wait.
 * RU: Ждать predicate вне допуска GC, сохраняя Prog/REG потока. Восстановить
 * допуск до возврата; при ожидании не читать управляемую память. */
static void wait_for(int *predicate)
{
    int token = dvm_runtime_gc_native_try_leave();
    require(token == 1, "test wait releases admission");
    pthread_mutex_lock(&gate);
    while (!*predicate)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    dvm_runtime_gc_native_enter(token);
}


/* EN: Delay only the table-waiting child, before its genuine cancellation.
 * Parent cancellation proceeds on its own owner. No production lock is held.
 * RU: Задержать лишь ребёнка, ожидающего таблицу, до настоящей отмены.
 * Отмена родителя идёт на его потоке; production-lock не удерживается. */
int __wrap_dvm_bigtable_cancel_wait(Prog *child)
{
    if (dvm_runqueue_observed_state(child) == DVM_TABLE_WAIT)
    {
        pthread_mutex_lock(&gate);
        require(!held, "one table cancellation is delayed");
        held = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&gate);
        wait_for(&released);
    }
    if (child->pid == 3 && ++child_calls == 2)
    {
        wait_for(&first_applied);
    }
    return __real_dvm_bigtable_cancel_wait(child);
}


/* EN: Count calls by the middle parent (PID 2); release A only after B was
 * published to the same child. Preserve the real result and ownership.
 * RU: Считать вызовы среднего родителя (PID 2); разрешить A только после
 * публикации B тому же ребёнку. Сохранить настоящий результат и владение. */
int __wrap_dvm_runtime_prepare_cancelwait(Prog *parent, int pid)
{
    int attempt = 0;
    if (parent->pid == 2)
    {
        pthread_mutex_lock(&gate);
        attempt = ++attempts;
        pthread_mutex_unlock(&gate);
    }
    int result = __real_dvm_runtime_prepare_cancelwait(parent, pid);
    if (attempt == 2)
    {
        pthread_mutex_lock(&gate);
        require(held && !released && result == 0, "B is parked before A completes");
        released = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&gate);
        puts("PASS second cancellation posted before first completion");
    }
    return result;
}


/* EN: Capacity 101 is the grandparent's marker: wait for the delayed A request,
 * then invoke the real constructor with borrowed operands and unchanged result.
 * RU: Capacity 101 — метка деда: дождаться задержки A, затем вызвать настоящий
 * конструктор с заимствованными аргументами и исходным результатом. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity, void (*mover)(void), int guarded)
{
    if (capacity == 101)
    {
        wait_for(&held);
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, guarded);
}


/* EN: Take the real node from borrowed endpoint/reply FIFO with original wait
 * policy. Return it unchanged, observing only its exclusively owned native fields.
 * This also covers staging outside GC admission; the observation is thread-local.
 * RU: Забрать настоящий узел из endpoint/reply FIFO с исходным режимом wait.
 * Вернуть без изменений, читая лишь принадлежащие получателю нативные поля.
 * Это покрывает извлечение вне допуска GC; наблюдение хранится в TLS. */
DVM_OwnerMessage *__wrap_dvm_messages_take(DVM_MessageEndpoint *endpoint, int replies, int wait)
{
    DVM_OwnerMessage *reply = __real_dvm_messages_take(endpoint, replies, wait);
    if (reply && reply->kind == DVM_PROG_MESSAGE_CANCEL_RESULT && reply->sender == 3)
    {
        saw_reply = 1;
    }
    return reply;
}


/* EN: Borrow parent (possibly NULL for the rootless controller). Release B only
 * after A has been applied or discarded by the real consumer.
 * The reply itself may already be freed. Arguments stay borrowed.
 * RU: Заимствовать parent (возможно NULL у controller без гостя). Разрешить B лишь
 * после принятия либо отбрасывания A настоящим получателем.
 * Сам ответ уже может быть освобождён. Аргументы остаются заимствованными. */
void __wrap_dvm_managed_workers_finish_reply(Prog *parent)
{
    __real_dvm_managed_workers_finish_reply(parent);
    if (parent && parent->pid == 2 && saw_reply)
    {
        pthread_mutex_lock(&gate);
        first_applied = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&gate);
    }
}


/* EN: At normal process exit all unique cancellation nodes must be freed.
 * Native threads have joined; fail on missing disposal, without managed access.
 * RU: При выходе все узлы отмены должны быть освобождены. Native-потоки уже
 * присоединены; отсутствие освобождения — ошибка, managed-память не читается. */
static void check_storage(void)
{
    require(allocations == 3 && disposals == allocations, "all three operations disposed exactly once");
    puts("PASS cancellation storage sent=3 freed=3 live=0");
}


/* EN: Track a native cancellation node before ownership transfer. Repeated send
 * of the same node as a reply is not a new allocation. Preserve the real result.
 * RU: Учесть узел отмены до передачи владения. Ответ в том же узле — не новая
 * память. Сохранить настоящий результат отправки. */
int __wrap_dvm_messages_send(DVM_MessageRouter *router, uintptr_t destination, int reply, DVM_OwnerMessage *node)
{
    if (node->kind == DVM_PROG_MESSAGE_CANCEL_WAIT)
    {
        pthread_mutex_lock(&gate);
        require(allocations < 8, "bounded cancellation allocation witness");
        if (!allocations)
        {
            require(atexit(check_storage) == 0, "register final storage check");
        }
        nodes[allocations++] = node;
        pthread_mutex_unlock(&gate);
    }
    return __real_dvm_messages_send(router, destination, reply, node);
}


/* EN: Count disposal of tracked native nodes before forwarding the exact free.
 * A cleared slot cannot match a later free; no allocation contents are inspected.
 * RU: Учесть удаление нативного узла до настоящего free. Очищенный слот уже не
 * совпадёт с повторным free; содержимое памяти не читается. */
void __wrap_free(void *pointer)
{
    if (pointer)
    {
        pthread_mutex_lock(&gate);
        for (int index = 0; index < allocations; index++)
        {
            if (nodes[index] == pointer)
            {
                nodes[index] = NULL;
                disposals++;
                break;
            }
        }
        pthread_mutex_unlock(&gate);
    }
    __real_free(pointer);
}
