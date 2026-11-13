/* Copyright (c) 2026 bhgv
 * EN: Expose a scalar preflight/payload gap using actual guards and VM calls.
 * RU: Воспроизвести разрыв preflight/payload настоящими guard и вызовами VM.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "runtime_gc.h"
#include "disvm/native_context.h"
#include "table_request.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER;  /* Protect one controlled native overlap. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Peer finished admission inspection. */
static Bigtable_Handle *parked_table;                /* Observed early waiter; fixture's shared table remains rooted. */
static FvmOwned *held_guard;                         /* Borrowed actual native payload guard. */
static int selected;                                 /* Atomic one-shot head selection. */
static int released;                                 /* Peer parked or exposed the old busy error. */
static _Thread_local Bigtable_Handle *current_table; /* Captured table for the selected scalar call. */


int __real_dvm_bigtable_call_ready(void (*)(void *), void *);


int __real_fvm_table_request(FvmTableOwner *, uint64_t, FvmTableRequest *);


int __real_fvm_owned_try_enter(FvmOwned *, uint64_t);


/* EN: Fail precisely; condition/message refer only to native fixture state.
 * RU: Завершить с точной ошибкой; condition/message относятся только к тесту. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL table preflight: %s\n", message);
        exit(12);
    }
}


/* EN: Observe a real competing preflight. Parking must precede native payload entry.
 * function/frame are borrowed; no instruction, frame or guard is replaced.
 * RU: Наблюдать настоящий preflight соседа. Ожидание должно предшествовать payload.
 * function/frame заимствованы; инструкции, кадры и guard не заменяются. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    Bigtable_Handle *table = NULL;
    if (function == Bigtable_setindex)
    {
        table = ((F_Bigtable_setindex *)frame)->tab;
        current_table = table;
    }
    int ready = __real_dvm_bigtable_call_ready(function, frame);
    if (table && !ready)
    {
        pthread_mutex_lock(&sync);
        /* EN: A peer may park before the admitted head reaches its payload
         * wrapper. Retain that observation, not just a condition notification.
         * RU: Партнёр может припарковаться до входа головы в payload-wrapper.
         * Сохранить наблюдение, а не только уведомление condvar. */
        parked_table = table;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&sync);
    }
    return ready;
}


/* EN: Detect the obsolete busy result on the actual competing payload guard.
 * guard/token are forwarded; release the paused head before the VM reports it.
 * RU: Обнаружить прежний busy на настоящем guard соседа.
 * guard/token передаются; снять барьер до обработки ошибки VM. */
int __wrap_fvm_owned_try_enter(FvmOwned *guard, uint64_t token)
{
    int result = __real_fvm_owned_try_enter(guard, token);
    if (result == EBUSY)
    {
        pthread_mutex_lock(&sync);
        if (guard == held_guard)
        {
            released = 1;
            pthread_cond_broadcast(&changed);
        }
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* EN: Pause one real child SET after admission while a competing resident arrives.
 * Borrow owner/token/request through the real request. Publish actual native roots
 * while waiting so the private barrier cannot block a GC rendezvous.
 * RU: Задержать один SET ребёнка после допуска до прихода другого потока.
 * owner/token/request заимствованы до возврата. Опубликовать настоящие корни
 * на время ожидания, чтобы частный барьер не блокировал rendezvous GC. */
int __wrap_fvm_table_request(FvmTableOwner *owner, uint64_t token, FvmTableRequest *request)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (request->operation == FVM_TABLE_SET && process && process->pid > 1 &&
        !__atomic_exchange_n(&selected, 1, __ATOMIC_ACQ_REL))
    {
        pthread_mutex_lock(&sync);
        held_guard = &owner->guard;
        pthread_mutex_unlock(&sync);
        int native = dvm_runtime_gc_native_try_leave();
        require(native > 0, "paused payload publishes its native roots");
        struct timespec deadline;
        require(!clock_gettime(CLOCK_REALTIME, &deadline), "native barrier clock");
        deadline.tv_sec += 30;
        pthread_mutex_lock(&sync);
        /* EN: Wait on durable state, even if every peer parked before this
         * wrapper published its guard. No notification itself is the proof.
         * RU: Ждать сохранённый факт, даже если все партнёры припарковались
         * до публикации guard. Само уведомление не является подтверждением. */
        while (!released && parked_table != current_table)
        {
            int result = pthread_cond_timedwait(&changed, &sync, &deadline);
            require(result == 0 || result == ETIMEDOUT, "native barrier wait");
            if (result == ETIMEDOUT)
            {
                require(released || parked_table == current_table,
                        "no competing preflight observed before bounded deadline");
            }
        }
        fprintf(stderr, "TABLE_PREFLIGHT parked_history=%d\n", parked_table == current_table);
        held_guard = NULL;
        pthread_mutex_unlock(&sync);
        dvm_runtime_gc_native_enter(native);
        fprintf(stderr, "PASS table primitive admission overlap exercised\n");
    }
    return __real_fvm_table_request(owner, token, request);
}
