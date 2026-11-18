/* Copyright (c) 2026 bhgv
 * EN: Complete nested-envelope validation/staging, immutable recipient capture and atomic FIFO publication.
 * RU: Проверка/staging полных nested envelopes, захват адресатов и атомарная FIFO publication.
 */
#include "internal.h"


/* EN/RU: Transaction-local copied messages; scopefeed owns all nodes until complete envelope commits.
 * Копии transaction-local messages принадлежат scopefeed до commit всего envelope.
 */
typedef struct Admission
{
    ScopeMessage *head; /* Owned staged chain. */
    ScopeMessage *tail; /* Borrowed newest staged node. */
    size_t count;       /* Total staged messages, checked against hub accounting. */
    int reason;         /* Refusal reason, PROTOCOL or NOMEM. */
} Admission;


/* EN/RU: Append one resolved copied record, caller holds mapping lock; no recipient can run or block admission.
 * Добавить resolved copied record под mapping lock, без запуска либо ожидания recipient.
 */
static int stage(Admission *admission, StyxScope *scope, unsigned long tag, const void *bytes, size_t size)
{
    if (size > (size_t)-1 - sizeof(ScopeMessage) || admission->count == (size_t)-1 ||
        scope->hub->queued > (size_t)-1 - admission->count - 1)
    {
        admission->reason = STYX_CONTROL_NOMEM;
        return 0;
    }
    ScopeMessage *message = malloc(sizeof(*message) + size); /* Private transaction copy, no mailbox mutation yet. */
    if (!message)
    {
        admission->reason = STYX_CONTROL_NOMEM;
        return 0;
    }
    message->next = NULL;
    message->target = scope;
    message->tag = tag;
    message->size = size;
    if (size)
    {
        memcpy(message->bytes, bytes, size);
    }
    if (admission->tail)
    {
        admission->tail->next = message;
    }
    else
    {
        admission->head = message;
    }
    admission->tail = message;
    admission->count++;
    return 1;
}


/* EN/RU: Route one group record under immutable transaction mapping; only tag0 targets this group's services.
 * Clients parse their envelope once, payloads stay opaque. Groups recurse only through explicitly granted children.
 * Маршрутизировать group record; tag0 лишь service данного group, leaves opaque, recursion лишь явных children.
 */
static int route(Admission *admission, StyxScope *scope, unsigned long tag, const unsigned char *bytes, size_t size)
{
    if (!tag)
    {
        return stage(admission, scope, 0, bytes, size);
    }
    StyxScope *target = scope->children; /* Native-only direct-child namespace, root tags are not globally visible. */
    while (target && target->tags[STYX_SCOPE_RX] != tag)
    {
        target = target->sibling;
    }
    if (!target)
    {
        return 0;
    }
    size_t position = 0; /* Complete envelope payload, never an implicit cross-envelope fragment. */
    while (position < size)
    {
        FVM_SRecord record; /* Verified borrowed inner view, private to this transaction. */
        size_t used = 0; /* Exact complete frame extent. */
        if (fvm_srecord_read(bytes + position, size - position, scope->hub->limit, &record, &used) != FVM_S_OK || !used)
        {
            return 0;
        }
        int accepted = target->leaf ? stage(admission, target, record.tag, record.payload, record.size) :
            route(admission, target, record.tag, record.payload, record.size);
        if (!accepted)
        {
            return 0;
        }
        position += used;
    }
    return 1;
}


/* EN/RU: Enforce outer tag plus payload body even when the underlying control permits larger records.
 * No allocation or transport I/O. / Проверить outer tag+payload body, даже при большем limit control.
 */
static int fits(ScopeHub *hub, unsigned long tag, size_t size)
{
    FVM_SInteger number; /* Canonical original tag integer, not a native-width serialized field. */
    number.high = 0;
    number.low = tag;
    number.negative = 0;
    unsigned char encoded[FVM_SRECORD_INTEGER_MAX]; /* Bounded tag encoding only. */
    size_t used = 0; /* Exact tag bytes charged to body limit. */
    return fvm_sinteger_write(&number, encoded, sizeof(encoded), &used) == FVM_S_OK &&
        used <= hub->limit && size <= hub->limit - used;
}


/* EN/RU: See internal.h; mapping changes serialize with the whole outer frame, never with consumer progress.
 * См. internal.h; mapping changes сериализуются с outer frame целиком, не с consumer progress.
 */
int scopefeed(ScopeHub *hub, unsigned long tag, const void *bytes, size_t size)
{
    Admission admission = {NULL, NULL, 0, STYX_CONTROL_PROTOCOL}; /* Whole-frame atomic publication transaction. */
    pthread_mutex_lock(&hub->lock);
    int live = !hub->reason; /* No publication after retirement. */
    int accepted = live && tag <= 0xffffffffUL && (!size || bytes) && fits(hub, tag, size) &&
        route(&admission, hub->root, tag, bytes, size);
    while (admission.head)
    {
        ScopeMessage *message = admission.head; /* Transfer one staged node. */
        admission.head = message->next;
        if (accepted)
        {
            StyxScope *target = message->target; /* Captured destination, never re-resolved by a consumer. */
            message->next = NULL;
            if (target->tail)
            {
                target->tail->next = message;
            }
            else
            {
                target->head = message;
            }
            target->tail = message;
            target->count++;
            hub->queued++;
        }
        else
        {
            free(message);
        }
    }
    if (accepted)
    {
        pthread_cond_broadcast(&hub->changed);
    }
    pthread_mutex_unlock(&hub->lock);
    if (live && !accepted)
    {
        scopeend(hub, admission.reason);
    }
    return accepted;
}
