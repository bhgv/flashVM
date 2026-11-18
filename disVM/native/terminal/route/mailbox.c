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
    unsigned long tag;       /* Original S-record tag; UI keeps it as private metadata. */
    size_t size;             /* Exact copied payload extent, zero permitted. */
    unsigned char data[];    /* Owned exact payload storage immediately after native metadata. */
};


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
        if (mailbox->count > mailbox->peak_count)
        {
            mailbox->peak_count = mailbox->count;
        }
        if (mailbox->bytes > mailbox->peak_bytes)
        {
            mailbox->peak_bytes = mailbox->bytes;
        }
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
int routemailboxnext(RouteMailbox *mailbox, int terminal, unsigned long *tag,
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
