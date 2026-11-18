/* Copyright (c) 2026 bhgv
 * EN: Dynamically allocated per-recipient messages with FIFO, accounting and explicit exhaustion failure.
 * RU: Динамические сообщения адресатов с FIFO, учётом и явным отказом при исчерпании памяти.
 */
#include "internal.h"


/* EN/RU: Native copied record, owned by one mailbox until popped or disposed; no managed pointers.
 * Native record-копия принадлежит одной FIFO до pop/dispose, без managed pointers.
 */
struct RouteRecord
{
    struct RouteRecord *next; /* Owned continuation, NULL for newest accepted record. */
    unsigned long tag;        /* Original S-record tag; UI keeps it as private metadata. */
    size_t size;              /* Exact copied payload extent, zero permitted. */
    struct timespec created;  /* Monotonic admission timestamp, private diagnostic only. */
    int timed;                /* Timestamp capture succeeded; never a permission or lifetime marker. */
    unsigned char data[];     /* Owned exact payload storage immediately after native metadata. */
};


/* EN/RU: Add one diagnostic total under owner lock; saturate on overflow without failing admission.
 * Добавить diagnostic total под lock; overflow насыщает counter, не отказывая admission.
 */
static void total(RouteMailbox *mailbox, size_t *counter, size_t addition)
{
    if (addition > (size_t)-1 - *counter)
    {
        *counter = (size_t)-1;
        mailbox->saturated = 1;
    }
    else
    {
        *counter += addition;
    }
}


/* EN/RU: See mailbox.h; allocation/copy outside lock, publication is atomic with stop and accounting.
 * См. mailbox.h; allocation/copy вне lock, publication атомарна со stop и accounting.
 */
int routeenqueue(StyxBidirRoute *route, RouteMailbox *mailbox, unsigned long tag,
    const void *bytes, size_t size)
{
    struct RouteRecord *record = size <= (size_t)-1 - sizeof(*record) ? malloc(sizeof(*record) + size) : NULL;
    if (!record)
    {
        routeend(route, STYX_CONTROL_NOMEM);
        return 0;
    }
    record->next = NULL;
    record->tag = tag;
    record->size = size;
    if (size)
    {
        memcpy(record->data, bytes, size);
    }
    pthread_mutex_lock(&route->lock);
    int exhausted = mailbox->count == (size_t)-1 || size > (size_t)-1 - mailbox->bytes; /* Guard accounting before mutation. */
    int accepted = !route->reason && !exhausted; /* Stop cannot publish a new record. */
    if (accepted)
    {
        record->timed = clock_gettime(CLOCK_MONOTONIC, &record->created) == 0;
        if (mailbox->tail)
        {
            mailbox->tail->next = record;
        }
        else
        {
            mailbox->head = record;
        }
        mailbox->tail = record;
        mailbox->count++;
        mailbox->bytes += size;
        total(mailbox, &mailbox->received, 1);
        total(mailbox, &mailbox->received_bytes, size);
        if (mailbox->count > mailbox->peak_count)
        {
            mailbox->peak_count = mailbox->count;
        }
        if (mailbox->bytes > mailbox->peak_bytes)
        {
            mailbox->peak_bytes = mailbox->bytes;
        }
        routepressureupdate(route, mailbox, 0);
        pthread_cond_broadcast(&route->changed);
    }
    pthread_mutex_unlock(&route->lock);
    if (!accepted)
    {
        free(record);
        if (exhausted)
        {
            routeend(route, STYX_CONTROL_NOMEM);
        }
    }
    return accepted;
}


/* EN/RU: See mailbox.h; release only the successfully copied head, retaining FIFO and exact counters.
 * См. mailbox.h; освободить лишь успешно скопированный head, сохраняя FIFO и точные counters.
 */
int routemailboxnext(StyxBidirRoute *route, RouteMailbox *mailbox, int terminal, unsigned long *tag,
    void *bytes, size_t capacity, size_t *size)
{
    struct RouteRecord *record = mailbox->head; /* Borrowed under caller-held route mutex. */
    if (!record)
    {
        return terminal ? STYX_CONTROL_END : STYX_CONTROL_EMPTY;
    }
    *size = record->size;
    if (record->size > capacity)
    {
        return STYX_CONTROL_SMALL;
    }
    if (record->size)
    {
        memcpy(bytes, record->data, record->size);
    }
    *tag = record->tag;
    mailbox->head = record->next;
    if (!mailbox->head)
    {
        mailbox->tail = NULL;
    }
    mailbox->count--;
    mailbox->bytes -= record->size;
    total(mailbox, &mailbox->delivered, 1);
    total(mailbox, &mailbox->delivered_bytes, record->size);
    routepressureupdate(route, mailbox, 0);
    free(record);
    return STYX_CONTROL_RECORD;
}


/* EN/RU: See mailbox.h; final disposal with no concurrent producers/readers, historical counters reset.
 * См. mailbox.h; final disposal без concurrent producers/readers, historical counters сбрасываются.
 */
void routemailboxclear(RouteMailbox *mailbox)
{
    while (mailbox->head)
    {
        struct RouteRecord *record = mailbox->head; /* Owned node transferred out of chain. */
        mailbox->head = record->next;
        free(record);
    }
    memset(mailbox, 0, sizeof(*mailbox));
}


/* EN/RU: See mailbox.h; monotonic age cannot be made negative by a failed/regressing clock.
 * См. mailbox.h; monotonic age не становится отрицательным при failed/regressing clock.
 */
int routemailboxage(RouteMailbox *mailbox, unsigned long long *milliseconds)
{
    struct timespec now; /* Independent current monotonic reading. */
    struct RouteRecord *record = mailbox->head; /* Borrowed under caller-held route mutex. */
    if (!record || !record->timed || clock_gettime(CLOCK_MONOTONIC, &now) || now.tv_sec < record->created.tv_sec ||
        (now.tv_sec == record->created.tv_sec && now.tv_nsec < record->created.tv_nsec))
    {
        return 0;
    }
    unsigned long long seconds = (unsigned long long)(now.tv_sec - record->created.tv_sec); /* Nonnegative bounded difference. */
    long nanos = now.tv_nsec - record->created.tv_nsec; /* Borrow one second if needed. */
    if (nanos < 0)
    {
        seconds--;
        nanos += 1000000000L;
    }
    unsigned long long fraction = (unsigned long long)nanos / 1000000ULL; /* Truncate fractional milliseconds. */
    *milliseconds = seconds > (~0ULL - fraction) / 1000ULL ? ~0ULL : seconds * 1000ULL + fraction;
    return 1;
}
