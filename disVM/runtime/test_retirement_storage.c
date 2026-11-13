/* Copyright (c) 2026 bhgv
 * EN: Keep stale native mailbox debt ahead of two real child retirements.
 * RU: Удержать старые native-ответы перед двумя настоящими завершениями детей.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_gc.h"
#include "managed_workers.h"
#include "runqueue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;  /* Test coordination only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Predicate notification. */
static __thread int reserve_attempt;                      /* Parent-local count of A preflights. */
static __thread int refusing;                             /* Parent-local reservation allocation boundary. */
static __thread int refused;                              /* One deterministic native allocation refusal. */
static int held;                                          /* Child A reached retirement, under gate. */
static int release_a;                 /* Parent has observed no reply and will consume cancellation, under gate. */
static int published_a;               /* A and preceding native debt are queued, under gate. */
static int published_b;               /* B is queued before parent leaves its prepared call, under gate. */
static void *tracked[34];             /* Two retirement nodes and 32 explicit debt nodes, under gate. */
static int tracked_count;             /* Recorded ownership transfers, under gate. */
static int freed_count;               /* Exact recorded disposals, under gate. */
static DVM_OwnerMessage *first_reply; /* Native address only; A owns it until publication. */


/* EN: Production linker targets; borrowed inputs and exact results are preserved.
 * RU: Production-цели линкера; заимствованные входы и настоящие результаты сохранены. */
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);
int __real_dvm_runqueue_begin_retire(Prog *child);
int __real_dvm_messages_pending(DVM_MessageEndpoint *, int, unsigned);
int __real_dvm_messages_send(DVM_MessageRouter *, uintptr_t, int, DVM_OwnerMessage *);
int __real_dvm_runtime_prepare_killwait(Prog *, int);
Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), int);


/* EN: Fail precisely with borrowed diagnostic before corrupting the real router.
 * RU: Завершить тест с точной заимствованной диагностикой до порчи настоящей очереди. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL retirement storage: %s\n", message);
        fflush(stderr);
        _Exit(12);
    }
}


/* EN: Require exact disposal after native owners join; no managed data accessed.
 * RU: После join native-владельцев проверить точное освобождение без managed-доступа. */
static void check_storage(void)
{
    require(tracked_count == 34 && freed_count == 34, "all retirement and debt allocations disposed");
    puts("PASS retirement storage allocations=34 freed=34 live=0");
}


/* EN: Record borrowed pointer before its ownership transfer, under gate.
 * Only the first record registers final verification; capacity refusal fails test.
 * RU: Учесть заимствованный pointer до передачи владения под gate. Первая запись
 * включает итоговую проверку; исчерпание вместимости завершает тест с ошибкой. */
static void track(void *pointer)
{
    pthread_mutex_lock(&gate);
    require(tracked_count < 34, "bounded allocation witness");
    if (!tracked_count)
    {
        require(atexit(check_storage) == 0, "register final storage verification");
    }
    tracked[tracked_count++] = pointer;
    pthread_mutex_unlock(&gate);
}


/* EN: CONTROL=2 refuses only the first A reply-reserve allocation, before
 * parking. Forward all other count/size pairs with the real allocator contract.
 * RU: CONTROL=2 отклоняет лишь первое выделение резерва A до парковки.
 * Все иные count/size передаются настоящему аллокатору с исходным контрактом. */
void *__wrap_calloc(size_t count, size_t size)
{
    if (refusing && !refused && count == 1 && size == sizeof(DVM_OwnerMessage))
    {
        refused = 1;
        return NULL;
    }
    return __real_calloc(count, size);
}


/* EN: Count disposal of a tracked node before the exact real free, never read
 * allocation contents. Other pointers and NULL forward unchanged.
 * RU: Учесть удаление известного узла до настоящего free, не читая его поля.
 * Иные указатели и NULL передаются без изменений. */
void __wrap_free(void *pointer)
{
    if (pointer)
    {
        pthread_mutex_lock(&gate);
        for (int index = 0; index < tracked_count; index++)
        {
            if (tracked[index] == pointer)
            {
                tracked[index] = NULL;
                freed_count++;
                break;
            }
        }
        pthread_mutex_unlock(&gate);
    }
    __real_free(pointer);
}


/* EN: Wait for predicate under gate. release permits native leave/reentry only
 * at a published runnable/rootless boundary. Retirement and prepared-wait probes
 * retain their existing admission and roots; they hold no production mutex.
 * No managed data is accessed while waiting; other mutators remain admitted.
 * RU: Ждать predicate под gate. release разрешает leave/reentry лишь на границе
 * опубликованного runnable либо без гостя. Retirement и prepared-wait сохраняют
 * исходный допуск и корни без production-mutex. При ожидании нет managed-доступа;
 * другие mutator продолжают работать. */
static void wait_for(int *predicate, int release)
{
    int token = release ? dvm_runtime_gc_native_try_leave() : 0;
    require(!release || token == 1, "test wait releases admission");
    pthread_mutex_lock(&gate);
    while (!*predicate)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    if (release)
    {
        dvm_runtime_gc_native_enter(token);
    }
}


/* EN: Hold borrowed A before its real lifetime seal; retirement takes no queue
 * mutex here. Return the actual seal result on the same permanent owner.
 * RU: Задержать A перед настоящим lifetime seal без mutex очереди. Вернуть
 * настоящий результат закрытия на том же постоянном владельце. */
int __wrap_dvm_runqueue_begin_retire(Prog *child)
{
    if (child->pid == 3)
    {
        pthread_mutex_lock(&gate);
        held = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&gate);
        wait_for(&release_a, 0);
    }
    return __real_dvm_runqueue_begin_retire(child);
}


/* EN: Preserve a genuinely empty reply observation, but publish A before the
 * owner continues to its already queued cancellation request. Borrow endpoint;
 * return the real observation, never hide a message present at the actual poll.
 * RU: Сохранить настоящий пустой снимок ответов, но опубликовать A до обработки
 * уже поставленной отмены. Endpoint заимствован; вернуть настоящий снимок,
 * не скрывая сообщение, присутствовавшее при фактической проверке. */
int __wrap_dvm_messages_pending(DVM_MessageEndpoint *endpoint, int replies, unsigned kind)
{
    int result = __real_dvm_messages_pending(endpoint, replies, kind);
    Prog *owner = dvm_managed_workers_guest();
    if (!result && replies && !kind && owner && owner->pid == 2 && ((DVM_RuntimeProcess *)owner)->exit_wait_pid == 3 &&
        __real_dvm_messages_pending(endpoint, 0, DVM_PROG_MESSAGE_CANCEL_WAIT))
    {
        pthread_mutex_lock(&gate);
        require(held && !release_a, "one empty reply observation");
        release_a = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&gate);
        wait_for(&published_a, 1);
    }
    return result;
}


/* EN: Prepend explicitly adversarial, owned scalar stale replies (ID zero,
 * never an issued cancellation). These model mailbox debt, not Limbo operations.
 * Then forward real A/B retirement nodes, requiring distinct ownership before
 * the second enqueue could corrupt the production queue. Inputs stay borrowed.
 * RU: Добавить явно тестовые владеемые старые скалярные ответы (ID ноль,
 * который никогда не выдаётся). Это долг mailbox, не операции Limbo. Затем
 * отправить настоящие A/B, требуя разную память до потенциальной порчи очереди.
 * Входы остаются заимствованными. */
int __wrap_dvm_messages_send(DVM_MessageRouter *router, uintptr_t destination, int replies, DVM_OwnerMessage *node)
{
    int retirement = replies && node->kind == DVM_PROG_MESSAGE_CHILD_RETIRED;
    uintptr_t sender = node->sender;
    if (retirement && sender == 3)
    {
        pthread_mutex_lock(&gate);
        first_reply = node;
        pthread_mutex_unlock(&gate);
        track(node);
        for (unsigned int index = 0; index < 32; index++)
        {
            DVM_OwnerMessage *stale = calloc(1, sizeof(*stale));
            require(stale != NULL, "native mailbox debt allocation");
            track(stale);
            stale->sender = 3;
            stale->kind = DVM_PROG_MESSAGE_CANCEL_RESULT;
            require(__real_dvm_messages_send(router, destination, 1, stale), "publish owned stale reply");
        }
    }
    if (retirement && sender == 4)
    {
        pthread_mutex_lock(&gate);
        require(node != first_reply, "B reused A's queued retirement node");
        pthread_mutex_unlock(&gate);
        track(node);
    }
    int result = __real_dvm_messages_send(router, destination, replies, node);
    if (retirement && (sender == 3 || sender == 4))
    {
        pthread_mutex_lock(&gate);
        if (sender == 3)
        {
            published_a = 1;
        }
        else
        {
            published_b = 1;
        }
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&gate);
    }
    return result;
}


/* EN: Keep B's real prepared call parked until B publishes its own reply. This
 * owner has released the registry mutex; arguments remain borrowed and result exact.
 * RU: Удержать настоящий подготовленный вызов B до публикации его ответа.
 * Владелец уже отпустил mutex реестра; аргументы заимствованы, результат настоящий. */
int __wrap_dvm_runtime_prepare_killwait(Prog *parent, int pid)
{
    if (CONTROL == 2 && parent->pid == 2 && pid == 3)
    {
        reserve_attempt++;
        if (reserve_attempt == 2)
        {
            require(refused && dvm_runqueue_observed_state(parent) == Pready &&
                        !((DVM_RuntimeProcess *)parent)->exit_wait_pid,
                    "reservation failure left caller runnable without an active wait");
            puts("PASS reservation refusal preserved runnable parent before retry");
        }
        refusing = reserve_attempt == 1;
    }
    int result = __real_dvm_runtime_prepare_killwait(parent, pid);
    refusing = 0;
    if (parent->pid == 2 && pid == 4 && !result)
    {
        wait_for(&published_b, 0);
        puts("PASS distinct retirement replies queued behind stale mailbox debt");
    }
    return result;
}


/* EN: Marker capacity 101 coordinates the grandparent with held A. Forward
 * all borrowed constructor operands and return its genuine result unchanged.
 * RU: Метка capacity 101 синхронизирует деда с задержанным A. Передать исходные
 * заимствованные аргументы конструктору и вернуть настоящий результат. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity, void (*mover)(void), int guarded)
{
    if (capacity == 101)
    {
        wait_for(&held, 1);
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, guarded);
}
