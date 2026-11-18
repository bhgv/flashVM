/* Copyright (c) 2026 bhgv
 * EN: Dynamic opaque typed S-record queues, independent of UI and application payload formats.
 * RU: Динамические очереди типизированных S-записей без зависимости от UI и форматов приложений.
 */
#include "internal.h"


/* EN/RU: See internal.h; bounded scan rejects whitespace, non-ASCII and unterminated identities.
 * См. internal.h; ограниченный просмотр отвергает пробелы, non-ASCII и слишком длинные identity.
 */
int routeschema(const char *type, unsigned long version)
{
    if (!type || !version || version > 0xffffffffUL)
    {
        return 0;
    }
    size_t length = 0; /* Bounded borrowed identity length. */
    while (length <= FVM_SBINDINGS_TYPE_MAX && type[length])
    {
        unsigned char value = (unsigned char)type[length]; /* Printable non-space schema byte. */
        if (value < 33 || value > 126)
        {
            return 0;
        }
        length++;
    }
    return length && length <= FVM_SBINDINGS_TYPE_MAX;
}


/* EN/RU: See internal.h; admission copies a record without waiting for its consumer.
 * См. internal.h; admission копирует запись без ожидания consumer.
 */
int routepublishtyped(StyxBidirRoute *route, RouteTypedQueue *queue, unsigned long tag, size_t size)
{
    return routeenqueue(route, &queue->mailbox, tag, route->scratch, size);
}


/* EN/RU: See types.h; copy and release exactly one accepted record, preserving SMALL.
 * См. types.h; скопировать и освободить одну запись, сохраняя SMALL.
 */
int styxbidirroutetypednext(StyxBidirRoute *route, unsigned index, unsigned long *tag,
    void *bytes, size_t capacity, size_t *size)
{
    if (!route || index >= route->typed_count || !tag || !size || (!bytes && capacity))
    {
        return STYX_CONTROL_END;
    }
    pthread_mutex_lock(&route->lock);
    int status = routemailboxnext(route, &route->typed[index].mailbox, route->reason, tag, bytes, capacity, size);
    pthread_mutex_unlock(&route->lock);
    return status;
}


/* EN/RU: See types.h; queue indices/configuration remain stable until external users retire.
 * См. types.h; индексы и конфигурация очередей неизменны до завершения внешних пользователей.
 */
int styxbidirroutetypedwait(StyxBidirRoute *route, unsigned index, unsigned timeout)
{
    if (!route || index >= route->typed_count)
    {
        return 0;
    }
    return routewaitcount(route, &route->typed[index].mailbox.count, timeout);
}


/* EN/RU: See types.h; queue configuration is copied before workers start and never mutated.
 * См. types.h; конфигурация скопирована до запуска workers и больше не меняется.
 */
int styxbidirroutetypedmatches(StyxBidirRoute *route, unsigned index, const char *type, unsigned long version)
{
    return route && index < route->typed_count && routeschema(type, version) &&
        route->typed[index].version == version && !strcmp(route->typed[index].type, type);
}


/* EN/RU: See types.h; outgoing authority is independent of every RX binding/queue.
 * См. types.h; исходящие права независимы от всех RX bindings/очередей.
 */
int styxbidirroutesendwait(StyxBidirRoute *route, unsigned long tag, const void *bytes, size_t size,
    unsigned timeout, struct DVM_Cancel *cancel)
{
    return route && !styxbidirroutestate(route) &&
        styxbidircontrolsendwait(route->control, tag, bytes, size, timeout, cancel);
}
