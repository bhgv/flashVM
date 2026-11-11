/* Copyright (c) 2026 bhgv
 * EN: Thread-local exception reserve and rooted retirement by Fast GC.
 * RU: Резерв исключений потока и его освобождение в Fast GC.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "kernel.h"
#include "exception_host.h"
#include "root_registry.h"
#include "runtime_gc.h"
#include "disvm/native_owner.h"
#include "gc/common/exception_queue.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>


static __thread DVM_ExceptionRetirement *reserve;                /* Current thread's optional owner. */
static pthread_mutex_t retired_lock = PTHREAD_MUTEX_INITIALIZER; /* Guards FIFO links only. */
static DVM_ExceptionRetirement *retired_head;                    /* First owner waiting for Fast. */
static DVM_ExceptionRetirement *retired_tail;                    /* Last owner waiting for Fast. */
static unsigned long shared_releases;                            /* Fast-only count, observed after its join. */
static unsigned long shared_with_peer;                           /* Shared releases overlapping an active guest. */
static unsigned long retained_releases;                          /* Retained fallback count, observed after join. */


/* [GC:] EN: Prepare entry under active heap admission; failure leaves it empty.
 * RU: Подготовить entry с допуском к куче. При отказе оставить пустым. */
static int prepare(void *argument)
{
    DVM_ExceptionRetirement *entry = argument;
    String *string = newstring(ERRMAX);
    if (!dvm_runtime_root_add(D2H(string)))
    {
        destroy(string);
        return 0;
    }
    entry->message = string;
    return 1;
}


/* [GC:] EN: Create the optional reserve once on this active executor.
 * RU: Один раз создать необязательный резерв текущего исполнителя. */
void dvm_runtime_exception_reserve_start(void)
{
    if (reserve)
    {
        return;
    }
    DVM_ExceptionRetirement *entry = calloc(1, sizeof(*entry));
    if (entry && kprotect(prepare, entry) && entry->message)
    {
        reserve = entry;
    }
    else
    {
        free(entry);
    }
}


/* [GC:] EN: Borrow the current thread's reserve for an admitted managed operation.
 * RU: Получить резерв текущего потока на время операции с допуском к куче. */
String *dvm_runtime_exception_reserve_value(void)
{
    return reserve ? reserve->message : NULL;
}


/* [GC:] EN: Transfer the rooted owner to Fast without touching the managed value.
 * RU: Передать владение с корнем в Fast без доступа к управляемому значению. */
void dvm_runtime_exception_reserve_stop(void)
{
    DVM_ExceptionRetirement *entry = reserve;
    if (!entry)
    {
        return;
    }
    reserve = NULL;
    entry->world = dvm_native_owner_world(dvm_native_owner_current());
    entry->epoch = entry->world ? dvm_world_retire_epoch(entry->world) : 0;
    pthread_mutex_lock(&retired_lock);
    if (retired_tail)
    {
        retired_tail->next = entry;
    }
    else
    {
        retired_head = entry;
    }
    retired_tail = entry;
    pthread_mutex_unlock(&retired_lock);
    dvm_runtime_gc_notify_fast();
}


/* [GC:] EN: Read queue readiness without entering NativeWorld.
 * RU: Проверить наличие работы без входа в NativeWorld. */
int dvm_runtime_exception_retirement_ready(void)
{
    pthread_mutex_lock(&retired_lock);
    int ready = retired_head != NULL;
    pthread_mutex_unlock(&retired_lock);
    return ready;
}


/* [GC:] EN: Release one reserve on the stopped-world Fast collector.
 * RU: Освободить один резерв на Fast GC при остановленной куче. */
void dvm_runtime_exception_retire_one(void)
{
    pthread_mutex_lock(&retired_lock);
    DVM_ExceptionRetirement *entry = retired_head;
    if (entry)
    {
        retired_head = entry->next;
        if (!retired_head)
        {
            retired_tail = NULL;
        }
    }
    pthread_mutex_unlock(&retired_lock);
    if (entry)
    {
        if (!dvm_runtime_root_remove(D2H(entry->message)))
        {
            abort();
        }
        destroy(entry->message);
        retained_releases++;
        free(entry);
    }
}


/* [GC:] EN: Report joined Fast counters; diagnostics controls printing. Call only
 * after worker join and producer shutdown. Does not reset process-lifetime counts.
 * RU: После join Fast и остановки производителей вывести счётчики при diagnostics.
 * Счётчики живут до завершения процесса и не сбрасываются.
 */
void dvm_runtime_exception_retirement_report(int diagnostics)
{
    if (diagnostics)
    {
        printf("NATIVEEXCEPTION shared=%lu with_peer=%lu retained=%lu pending=%d\n",
               shared_releases,
               shared_with_peer,
               retained_releases,
               dvm_runtime_exception_retirement_ready());
    }
}


/* [GC:] EN: Borrow the current FIFO head on the sole Fast consumer; see header.
 * RU: Получить голову FIFO на единственном потребителе Fast; контракт в заголовке.
 */
DVM_ExceptionRetirement *dvm_exception_retirement_peek(void)
{
    pthread_mutex_lock(&retired_lock);
    DVM_ExceptionRetirement *entry = retired_head;
    pthread_mutex_unlock(&retired_lock);
    return entry;
}


/* [GC:] EN: Remove the consumer-owned head entry without dropping its root.
 * RU: Снять принадлежащую потребителю голову entry, не снимая её корень.
 */
void dvm_exception_retirement_take(DVM_ExceptionRetirement *entry)
{
    pthread_mutex_lock(&retired_lock);
    if (!entry || retired_head != entry)
    {
        pthread_mutex_unlock(&retired_lock);
        fputs("disvm: emergency retirement consumer mismatch\n", stderr);
        abort();
    }
    retired_head = entry->next;
    if (!retired_head)
    {
        retired_tail = NULL;
    }
    pthread_mutex_unlock(&retired_lock);
}


/* [GC:] EN: Record a Fast-only release; peer identifies live-guest overlap.
 * RU: Учесть освобождение на Fast; peer означает перекрытие с активным гостем.
 */
void dvm_exception_retirement_count_shared(int peer)
{
    shared_releases++;
    shared_with_peer += peer != 0;
}
