/* Copyright (c) 2026 bhgv
 * EN: Process pins and acquire observation of published dispatch ownership.
 * RU: Удержания процессов и acquire-чтение опубликованного владельца исполнения.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runqueue_private.h"
#include "disvm/refcount.h"
#include "managed_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


/* Acquire a borrow from an independently live scheduler or queue reference.
 * RU: Добавить удержание отдельно живого process; при закрытом счётчике или переполнении отказать без изменения. */
int dvm_runqueue_pin(Prog *process)
{
    DVM_RuntimeProcess *native = runtime_process(process);
    int count;
    if (!native)
    {
        return 0;
    }
    do
    {
        count = dvm_refcount_load_i(&native->lifetime_holds);
        if (count < 0 || count == INT_MAX)
        {
            return 0;
        }
    } while (!dvm_refcount_compare_i(&native->lifetime_holds, count, count + 1));
    return 1;
}


/* Consume one owned borrow. No access to the Prog follows a successful drop.
 * RU: Снять принадлежащее вызывающему удержание process; после успешного снятия Prog не читается. */
void dvm_runqueue_unpin(Prog *process)
{
    DVM_RuntimeProcess *native = runtime_process(process);
    int count;
    if (!native)
    {
        abort();
    }
    do
    {
        count = dvm_refcount_load_i(&native->lifetime_holds);
        if (count <= 0)
        {
            abort();
        }
    } while (!dvm_refcount_compare_i(&native->lifetime_holds, count, count - 1));
}


/* Observe a live process. A negative count denotes closed admission.
 * RU: Прочитать число удержаний живого process; отрицательное значение означает закрытый допуск. */
int dvm_runqueue_hold_count(Prog *process)
{
    return dvm_refcount_load_i(&runtime_process(process)->lifetime_holds);
}


/* Seal admission exactly once before the scheduler dismantles the Prog.
 * RU: Один раз закрыть удержания process только при нулевом счётчике; вернуть результат атомарного сравнения. */
int dvm_runqueue_begin_retire(Prog *process)
{
    return process && dvm_refcount_compare_i(&runtime_process(process)->lifetime_holds, 0, -1);
}


/* EN: Acquire the published owner identity after its register snapshot writes.
 * RU: Прочитать владельца после публикации его записи регистров. */
DVM_NativeContext *dvm_runqueue_dispatch_owner(Prog *process)
{
    return process ? __atomic_load_n(&runtime_process(process)->dispatch_owner, __ATOMIC_ACQUIRE) : NULL;
}


/* EN: Observe ready publication of a separately retained process; NULL is false.
 * RU: Прочитать готовность отдельно удерживаемого process; NULL означает false. */
int dvm_runqueue_is_ready(Prog *process)
{
    return process ? __atomic_load_n(&runtime_process(process)->queued, __ATOMIC_ACQUIRE) : 0;
}
