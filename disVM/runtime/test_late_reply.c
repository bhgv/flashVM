/* Copyright (c) 2026 bhgv
 * EN: Hold a child reply across cancellation and native phases of its live parent.
 * RU: Задержать ответ ребёнка при отмене и native-переходах живого родителя.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "runqueue.h"
#include "owner_messages.h"
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;  /* Test predicates only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Predicate notification. */
static int held;                                          /* Child reply entered, protected by gate. */
static int released;                                      /* Parent abandoned its old wait, protected by gate. */
static int scanning; /* Child resumed admission before observing replies, under gate. */
static int finished; /* Real stale-reply callback returned, protected by gate. */


/* EN: Production linker targets; arguments retain their original ownership.
 * RU: Production-цели линкера; владение аргументами остаётся исходным. */
void __real_dvm_runtime_wait_request_finished(Prog *child, DVM_OwnerMessage *message);


Channel *__real_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity, void (*mover)(void), int guarded);


int __real_dvm_runtime_guest_sleep(unsigned long milliseconds);


/* EN: Check private protocol condition; borrow message and exit 10 on failure.
 * RU: Проверить condition частного протокола; message заимствован, сбой — exit 10. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL live parent late reply: %s\n", message);
        exit(10);
    }
}


/* EN: Wait outside GC participation for borrowed predicate under gate. The guest
 * and its roots stay owned by this thread. Restore the original admission token;
 * no managed pointer is accessed during the wait. Protocol failure exits 10.
 * RU: Ждать заимствованный predicate под gate вне участия в GC. Гость и корни
 * остаются на этом потоке. Восстановить исходный token; во время ожидания не
 * читать managed-память. Ошибка протокола завершает с 10. */
static void wait_for(int *predicate)
{
    int token = dvm_runtime_gc_native_try_leave();
    require(token == 1, "native wait releases admission");
    pthread_mutex_lock(&gate);
    while (!*predicate)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    dvm_runtime_gc_native_enter(token);
}


/* EN: Delay only the middle guest's borrowed child cancellation reply. The
 * grandparent's request (sender 1) must proceed so it can cancel the middle wait.
 * Preserve child/message lifetime through the real call; signal completion after
 * it returns. No VM fields are read while outside admission.
 * RU: Задержать лишь заимствованный ответ ребёнка среднему гостю. Запрос деда
 * (sender 1) пропустить для отмены ожидания среднего. Удержать child/message до
 * настоящего вызова, затем сообщить о завершении. Вне допуска поля VM не читать. */
void __wrap_dvm_runtime_wait_request_finished(Prog *child, DVM_OwnerMessage *message)
{
    if (message->kind != DVM_PROG_MESSAGE_CANCEL_WAIT || message->sender == 1)
    {
        __real_dvm_runtime_wait_request_finished(child, message);
        return;
    }
    pthread_mutex_lock(&gate);
    require(!held, "only one child reply is held");
    held = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    wait_for(&released);
    pthread_mutex_lock(&gate);
    scanning = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    /* EN: Amplify observation of the same stale, still-owned request. Its
     * correlation was withdrawn, so every real call must remain a no-op.
     * RU: Повторить наблюдение того же живого старого запроса. Его ожидание
     * отозвано: каждый настоящий вызов обязан оставить состояние неизменным. */
    for (unsigned int attempt = 0; attempt < 4096; attempt++)
    {
        __real_dvm_runtime_wait_request_finished(child, message);
        sched_yield();
    }
    pthread_mutex_lock(&gate);
    finished = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    puts("PASS child reply returned after parent abandoned wait");
}


/* EN: Capacity 101 is the guest's private coordination marker. Wait for the
 * held reply, then forward all borrowed constructor inputs and exact result.
 * RU: Capacity 101 — частная метка гостя. Дождаться задержанного ответа,
 * затем передать все заимствованные входы и исходный результат конструктора. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity, void (*mover)(void), int guarded)
{
    if (capacity == 101)
    {
        wait_for(&held);
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, guarded);
}


/* EN: Sentinel 42423 belongs only to the still-live parent after cancellation.
 * Release the stale reply before 256 real native leave/reentry pairs, then wait
 * for its completion. Check the same Prog, REG storage and values after every
 * pair. Other durations forward unchanged. Return one for the handled sentinel;
 * protocol failures exit 10. No parent-state read synchronizes the child callback
 * with these transitions, so restoring its old foreign read is a real TSan race.
 * RU: Метка 42423 принадлежит живому родителю после отмены. Разрешить старый ответ
 * до 256 настоящих leave/reentry, затем дождаться его завершения. После каждой
 * пары сверить Prog, память REG и значения. Иные длительности передать штатно.
 * Единица означает обработанную метку; сбой — exit 10. Чтение state родителя не
 * синхронизирует callback с переходами: возврат старого чтения даёт гонку TSan. */
int __wrap_dvm_runtime_guest_sleep(unsigned long milliseconds)
{
    if (milliseconds != 42423)
    {
        return __real_dvm_runtime_guest_sleep(milliseconds);
    }
    DVM_NativeContext *owner = dvm_native_context_current();
    Prog *parent = dvm_native_context_process(owner);
    REG *registers = &DVM_R;
    REG saved = DVM_R;
    require(parent && parent->pid != 1, "native sentinel belongs to the parent");
    pthread_mutex_lock(&gate);
    require(held && !released, "release follows cancellation of the parent wait");
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    wait_for(&scanning);
    for (unsigned int round = 0; round < 256; round++)
    {
        int token = dvm_runtime_gc_native_try_leave();
        require(token == 1, "parent enters native phase");
        sched_yield();
        dvm_runtime_gc_native_enter(token);
        require(dvm_native_context_current() == owner && dvm_native_context_process(owner) == parent &&
                    &DVM_R == registers && !memcmp(&saved, &DVM_R, sizeof(saved)),
                "late reply changed live parent continuation");
    }
    wait_for(&finished);
    puts("PASS live parent preserved Prog and REG across 256 native pairs");
    return 1;
}
