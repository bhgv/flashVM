/* Copyright (c) 2026 bhgv
 * EN: Hold an actual admitted table request through queued and owner killwait.
 * RU: Удержать настоящий запрос таблицы до killwait соседа и владельца.
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
#include <unistd.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Protect observed native lifetime. */
static FvmTableOwner *held_owner; /* Borrowed until the real request returns. */
static Bigtable_Handle *held_table; /* Rooted by the admitted primitive's frame/lease. */
static int held_pid; /* Numeric identity only, never a foreign Prog pointer. */
static unsigned phases; /* Numbered head/queued/kill boundary sequence. */
static _Thread_local Bigtable_Handle *current_table; /* Current setindex argument, owner-local. */

int __real_dvm_bigtable_call_ready(void (*)(void *), void *);
int __real_fvm_table_request(FvmTableOwner *, uint64_t, FvmTableRequest *);
int __real_dvm_runtime_prepare_killwait(Prog *, int);
int __real_fvm_table_owner_destroy(FvmTableOwner *);


/* EN: Stop with an exact semantic diagnostic before an unsafe native access.
 * RU: Завершить с точной семантической диагностикой до опасного нативного доступа. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL table scope kill: %s\n", message);
        exit(12);
    }
}


/* EN: Observe the real peer's FIFO admission result; never change its frame.
 * function/frame are borrowed from native MCALL preflight.
 * RU: Наблюдать результат допуска соседа в FIFO, не меняя его кадр.
 * function/frame заимствованы из настоящего preflight MCALL. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    Bigtable_Handle *table = NULL;
    if (function == Bigtable_setindex)
    {
        table = ((F_Bigtable_setindex *)frame)->tab;
        current_table = table;
    }
    int ready = __real_dvm_bigtable_call_ready(function, frame);
    if (!ready && table)
    {
        pthread_mutex_lock(&sync);
        if (table == held_table)
        {
            fprintf(stderr, "TABLE_SCOPE_QUEUED %u\n", phases);
        }
        pthread_mutex_unlock(&sync);
    }
    return ready;
}


/* EN: Hold the genuine admitted request while its killed frame/lease keeps it live.
 * owner/token/request borrow production inputs. Forward the real payload once,
 * validate its scalar result, then withdraw this native lifetime audit.
 * RU: Держать настоящий запрос, пока кадр/lease убиваемого потока хранит его жизнь.
 * Входы заимствованы. Выполнить настоящий payload один раз, проверить результат
 * и снять наблюдение времени жизни нативного владельца. */
int __wrap_fvm_table_request(FvmTableOwner *owner, uint64_t token, FvmTableRequest *request)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (!process || process->pid <= 1 || request->operation != FVM_TABLE_SET)
    {
        return __real_fvm_table_request(owner, token, request);
    }
    pthread_mutex_lock(&sync);
    require(held_owner == NULL, "queued primitive never reaches the native payload");
    held_owner = owner;
    held_table = current_table;
    held_pid = process->pid;
    unsigned phase = ++phases;
    pthread_mutex_unlock(&sync);
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "admitted primitive publishes its real frame and roots");
    fprintf(stderr, "TABLE_SCOPE_HEAD %u\n", phase);
    const char *descriptor = getenv("DVM_TABLE_SCOPE_RELEASE");
    require(descriptor != NULL, "native release pipe is inherited");
    char permit;
    ssize_t count;
    do
    {
        count = read(atoi(descriptor), &permit, 1);
    } while (count < 0 && errno == EINTR);
    require(count == 1 && permit == '1', "release follows accepted owner killwait");
    dvm_runtime_gc_native_enter(native);
    int result = __real_fvm_table_request(owner, token, request);
    int64_t value = 0;
    require(!result && request->status == FHT_OK &&
        fht_get(owner->payload, request->key, &value) == FHT_OK && value == 111,
        "admitted operation finishes once before owner retirement");
    pthread_mutex_lock(&sync);
    held_owner = NULL;
    held_table = NULL;
    held_pid = 0;
    pthread_mutex_unlock(&sync);
    return result;
}


/* EN: Announce only the parent's accepted wait for the held owner, after real
 * authorization/message publication. caller/pid follow production ownership.
 * RU: Сообщить только о принятом ожидании владельца после настоящей проверки
 * родительства и публикации сообщения. caller/pid следуют контракту runtime. */
int __wrap_dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    int result = __real_dvm_runtime_prepare_killwait(caller, pid);
    if (!result)
    {
        pthread_mutex_lock(&sync);
        if (pid == held_pid)
        {
            fprintf(stderr, "TABLE_SCOPE_OWNER_KILL %u\n", phases);
        }
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* EN: Reject finalization while the real native request still borrows its owner.
 * owner follows the actual finalizer's contract; no mock payload is supplied.
 * RU: Запретить финализацию, пока настоящий запрос заимствует owner.
 * owner следует контракту финализатора; макета payload нет. */
int __wrap_fvm_table_owner_destroy(FvmTableOwner *owner)
{
    pthread_mutex_lock(&sync);
    require(!owner || owner != held_owner, "native table finalized before its request returned");
    pthread_mutex_unlock(&sync);
    return __real_fvm_table_owner_destroy(owner);
}
