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
#include <pthread.h>
#include <stdlib.h>


/* EN: Own a managed reference and its root until Fast removes both.
 * RU: Владеть управляемой ссылкой и её корнем до освобождения в Fast. */
typedef struct ExceptionReserve
{
    struct ExceptionReserve *next; /* Native retirement FIFO link. */
    String *message;               /* Rooted managed value. */
} ExceptionReserve;

static __thread ExceptionReserve *reserve; /* Current thread's optional owner. */
static pthread_mutex_t retired_lock = PTHREAD_MUTEX_INITIALIZER; /* Guards FIFO links only. */
static ExceptionReserve *retired_head; /* First owner waiting for Fast. */
static ExceptionReserve *retired_tail; /* Last owner waiting for Fast. */


/* EN: Prepare entry under active heap admission; failure leaves it empty.
 * RU: Подготовить entry с допуском к куче. При отказе оставить пустым. */
static int prepare(void *argument)
{
    ExceptionReserve *entry = argument;
    String *string = newstring(ERRMAX);
    if (!dvm_runtime_root_add(D2H(string)))
    {
        destroy(string);
        return 0;
    }
    entry->message = string;
    return 1;
}


/* EN: Create the optional reserve once on this active executor.
 * RU: Один раз создать необязательный резерв текущего исполнителя. */
void dvm_runtime_exception_reserve_start(void)
{
    if (reserve)
    {
        return;
    }
    ExceptionReserve *entry = calloc(1, sizeof(*entry));
    if (entry && kprotect(prepare, entry) && entry->message)
    {
        reserve = entry;
    }
    else
    {
        free(entry);
    }
}


/* EN: Borrow the current thread's reserve for an admitted managed operation.
 * RU: Получить резерв текущего потока на время операции с допуском к куче. */
String *dvm_runtime_exception_reserve_value(void)
{
    return reserve ? reserve->message : NULL;
}


/* EN: Transfer the rooted owner to Fast without touching the managed value.
 * RU: Передать владение с корнем в Fast без доступа к управляемому значению. */
void dvm_runtime_exception_reserve_stop(void)
{
    ExceptionReserve *entry = reserve;
    if (!entry)
    {
        return;
    }
    reserve = NULL;
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


/* EN: Read queue readiness without entering NativeWorld.
 * RU: Проверить наличие работы без входа в NativeWorld. */
int dvm_runtime_exception_retirement_ready(void)
{
    pthread_mutex_lock(&retired_lock);
    int ready = retired_head != NULL;
    pthread_mutex_unlock(&retired_lock);
    return ready;
}


/* EN: Release one reserve on the stopped-world Fast collector.
 * RU: Освободить один резерв на Fast GC при остановленной куче. */
void dvm_runtime_exception_retire_one(void)
{
    pthread_mutex_lock(&retired_lock);
    ExceptionReserve *entry = retired_head;
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
        free(entry);
    }
}
