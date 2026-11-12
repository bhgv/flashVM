/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real producer publication between empty shared work and readiness.
 * RU: Настоящая публикация между пустым shared-проходом и проверкой готовности.
 */
#include "../host/gc_worker_private.h"
#include "disvm/native_owner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Own native handshakes until producer joins; no VM heap is mocked.
 * RU: Владеть native-синхронизацией до join производителя; куча VM не подменяется. */
typedef struct ReadinessCase
{
    FVM_PortSync *sync;     /* Guards producer publication and release. */
    DVM_NativeWorld *world; /* Actual admission domain through join. */
    int entered;            /* The real guest owns an active interval. */
    int sampled_empty;      /* Shared callback observed no pending work. */
    int pending;            /* Native backend work published after the sample. */
    int release;            /* Main allows guest exit after MORE. */
    int force_retained;     /* Genuine retained backend work after guest join. */
    unsigned int retained;  /* Actual retained callback count. */
} ReadinessCase;

static unsigned int quiescent_calls; /* Only the bound main collector invokes the probe. */


int __real_dvm_world_try_quiescent(DVM_NativeWorld *, DVM_NativeContext *);


/* EN: Require condition; borrowed message identifies a semantic failure.
 * RU: Проверить condition; заимствованный message называет нарушение. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL Fast readiness: %s\n", message);
        exit(12);
    }
}


/* [GC:] EN: Observe only the actual quiescent acquisition boundary.
 * Forward world/context unchanged when no active guest forbids the attempt.
 * RU: Наблюдать настоящую границу quiescent-захвата. Передать world/context
 * без изменения, когда активный гость не исключает эту попытку. */
int __wrap_dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    require(!dvm_world_has_active_guest(world), "active producer must keep Fast on shared retry");
    quiescent_calls++;
    return __real_dvm_world_try_quiescent(world, context);
}


/* [GC:] EN: Publish backend work after the real shared sample, while remaining
 * admitted without polling. argument borrows the case through native join.
 * RU: Опубликовать работу после настоящего shared-замера, оставаясь допущенным
 * без polling. argument заимствует состояние до native join. */
static void producer(void *argument)
{
    ReadinessCase *test = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long), NULL);
    require(context && dvm_native_context_bind(context), "producer context bound");
    DVM_HeapOwner *owner = dvm_native_owner_current();
    require(dvm_native_owner_attach(owner, test->world) && dvm_native_owner_enter(owner), "producer active admission");
    fvm_port_sync_lock(test->sync);
    test->entered = 1;
    fvm_port_sync_signal_all(test->sync);
    unsigned int waits = 0;
    while (!test->release && waits++ < 5000)
    {
        if (test->sampled_empty)
        {
            test->pending = 1;
            fvm_port_sync_signal_all(test->sync);
        }
        require(fvm_port_sync_wait_ms(test->sync, 1), "producer wait succeeds");
    }
    require(test->release, "producer release deadline");
    fvm_port_sync_unlock(test->sync);
    require(dvm_native_owner_leave(owner) && dvm_native_owner_detach(owner), "producer detached");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "producer context destroyed");
}


/* [GC:] EN: Execute real native backend work. context/budget are exact dispatch
 * inputs, user borrows the case. First sample is empty; the producer publishes
 * before return, so zero truthfully describes the sampled completed work.
 * RU: Выполнить работу native-backend. context/budget — параметры диспетчера,
 * user заимствует состояние. Первый замер пуст; производитель публикует до
 * возврата, поэтому ноль точно означает отсутствие выполненной работы. */
static unsigned int shared(DVM_NativeContext *context, size_t budget, void *user)
{
    ReadinessCase *test = user;
    require(context == dvm_native_context_current() && budget == 1, "shared dispatch inputs");
    fvm_port_sync_lock(test->sync);
    int completed = test->pending;
    if (!test->sampled_empty)
    {
        require(!completed && test->entered, "initial shared sample is actually empty");
        test->sampled_empty = 1;
        fvm_port_sync_signal_all(test->sync);
        unsigned int waits = 0;
        while (!test->pending && waits++ < 5000)
        {
            require(fvm_port_sync_wait_ms(test->sync, 1), "shared publication wait succeeds");
        }
        require(test->pending, "actual producer publishes before readiness");
    }
    else if (completed)
    {
        test->pending = 0;
    }
    fvm_port_sync_unlock(test->sync);
    return completed;
}


/* [GC:] EN: Read native backend readiness under its mutex; user stays borrowed.
 * RU: Прочитать готовность native-backend под его mutex; user заимствован. */
static int ready(void *user)
{
    ReadinessCase *test = user;
    fvm_port_sync_lock(test->sync);
    int result = test->pending || test->force_retained;
    fvm_port_sync_unlock(test->sync);
    return result;
}


/* [GC:] EN: Complete real retained backend work only after guest join.
 * context/budget are exact inputs; user borrows native state until return.
 * RU: Завершить резервную работу лишь после join гостя. context/budget — точные
 * параметры; user заимствует native-состояние до возврата. */
static DVM_GCWorkResult retained(DVM_NativeContext *context, size_t budget, void *user)
{
    ReadinessCase *test = user;
    DVM_WorldState state;
    require(context == dvm_native_context_current() && budget == 1 && dvm_world_state(test->world, &state) &&
                state.stopped && !state.active,
            "retained work uses actual stopped admission after join");
    test->retained++;
    test->force_retained = 0;
    return DVM_GC_DONE;
}


/* [GC:] EN: There are no mutator participants after join, hence no roots visit.
 * All visitor inputs are borrowed and must never reach this callback.
 * RU: После join участников-гостей нет, значит root-visitor не вызывается.
 * Все заимствованные параметры не должны попасть в этот обработчик. */
static void roots(DVM_NativeContext *context, void *registers, size_t size, void *process, void *user)
{
    (void)context;
    (void)registers;
    (void)size;
    (void)process;
    (void)user;
    require(0, "no visitor for detached producer");
}


/* [GC:] EN: Exercise the actual dispatcher in both poll modes. Native state
 * remains owned through join; no managed collector result is substituted.
 * RU: Проверить настоящий диспетчер в обоих режимах polling. Native-состояние
 * живёт до join; ответы управляемого сборщика не подменяются. */
int main(void)
{
    for (int locked = 0; locked < 2; locked++)
    {
        ReadinessCase test = {0};
        DVM_GCWorkers workers = {0};
        test.sync = fvm_port_sync_create();
        test.world = dvm_world_create_serial(locked);
        DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long), NULL);
        require(test.sync && test.world && context && dvm_native_context_bind(context), "native test setup");
        workers.config.world = test.world;
        workers.config.user = &test;
        workers.config.Fast.budget = 1;
        workers.config.Fast.run = retained;
        workers.config.fast_shared = shared;
        workers.config.fast_ready = ready;
        workers.config.fast_quiescent_only = 1;
        workers.config.roots = roots;
        workers.Fast.owner = &workers;
        workers.Fast.context = context;
        workers.Fast.kind = DVM_GC_FAST;
        FVM_PortThread *thread = fvm_port_thread_start(producer, &test);
        require(thread != NULL, "producer starts");
        fvm_port_sync_lock(test.sync);
        while (!test.entered)
        {
            require(fvm_port_sync_wait_ms(test.sync, 1000), "producer startup wait");
        }
        fvm_port_sync_unlock(test.sync);
        int executed;
        require(dvm_gc_worker_collect(&workers.Fast, GC_PASS_REQUEST, &executed) == DVM_GC_MORE,
                "newly published work remains on shared route");
        require(!test.retained && quiescent_calls == (unsigned)locked, "no retained probe with admitted producer");
        fvm_port_sync_lock(test.sync);
        test.release = 1;
        fvm_port_sync_signal_all(test.sync);
        fvm_port_sync_unlock(test.sync);
        fvm_port_thread_join(thread);
        require(dvm_gc_worker_collect(&workers.Fast, GC_PASS_REQUEST, &executed) == DVM_GC_DONE && !test.pending,
                "next shared slice consumes actual publication");
        test.force_retained = 1;
        require(dvm_gc_worker_collect(&workers.Fast, GC_PASS_REQUEST, &executed) == DVM_GC_DONE && test.retained == 1,
                "quiescent retained cleanup remains reachable");
        require(quiescent_calls == (unsigned)locked + 1, "one actual quiescent acquisition after join");
        require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "collector context cleanup");
        require(dvm_world_destroy(test.world), "world cleanup");
        fvm_port_sync_destroy(test.sync);
    }
    puts(
        "PASS Fast readiness: actual publication after empty sample; shared retry with active producer; retained after join");
    return 0;
}
