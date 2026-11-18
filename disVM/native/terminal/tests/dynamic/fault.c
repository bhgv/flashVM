/* Copyright (c) 2026 bhgv
 * EN: Observe final actual route admission and inject one real mailbox allocation failure on its owner.
 * RU: Наблюдать final actual route admission и один отказ allocation FIFO на её native owner.
 */
#include "../../route/internal.h"
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protect completion checkpoint only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* No sleeps or guest scheduling in fixture. */
static int done;                         /* Final command input admitted or terminal allocation refused. */
static __thread int allocation_refused;  /* Native dispatcher-local next malloc fault, no other thread affected. */


/* EN/RU: Original malloc, allocation ownership unchanged on non-injected calls.
 * Исходный malloc, ownership не меняется для обычных calls.
 */
extern void *__real_malloc(size_t);


/* EN/RU: Refuse exactly the armed native record allocation, never another worker's allocation.
 * Отказать ровно во включённой native record allocation, не затрагивая другие workers.
 */
void *__wrap_malloc(size_t size)
{
    if (allocation_refused)
    {
        allocation_refused = 0;
        return NULL;
    }
    return __real_malloc(size);
}


/* EN/RU: Original copied record admission, borrowed inputs through return.
 * Исходный admission копии записи, inputs borrowed до return.
 */
extern int __real_routeenqueue(StyxBidirRoute *, RouteMailbox *, unsigned long, const void *, size_t);


/* EN/RU: Observe last record after original admission; fault variants refuse the late typed marker's malloc.
 * Наблюдать последнюю запись после original admission; fault variants отказывают malloc late marker.
 */
int __wrap_routeenqueue(StyxBidirRoute *route, RouteMailbox *mailbox, unsigned long tag, const void *bytes, size_t size)
{
    if (STYX_DYNAMIC_CASE >= 3 && tag == 31)
    {
        allocation_refused = 1;
    }
    int result = __real_routeenqueue(route, mailbox, tag, bytes, size);
    if ((tag == 9 && size == 4 && !memcmp(bytes, "done", 4)) || (tag == 31 && !result))
    {
        pthread_mutex_lock(&lock);
        done = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&lock);
    }
    return result;
}


/* EN/RU: Native deterministic completion checkpoint, external CTest timeout bounds any failure.
 * Native deterministic completion checkpoint, внешний CTest timeout ограничивает failure.
 */
void dynamiccomplete(void)
{
    pthread_mutex_lock(&lock);
    while (!done)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
}
