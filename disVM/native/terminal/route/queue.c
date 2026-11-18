/* Copyright (c) 2026 bhgv
 * EN: One-item mailbox reads and monotonic native readiness, without callbacks or guest dispatch.
 * RU: Чтение по одной записи и monotonic-ожидание native-потребителя без guest dispatch.
 */
#define _POSIX_C_SOURCE 200809L
#include "internal.h"


/* EN/RU: See route.h; copy and release one accepted message under the queue mutex.
 * См. route.h; скопировать и освободить одно принятое сообщение под mutex очереди.
 */
int styxbidirrouteui(StyxBidirRoute *route, DVM_UIMessage *output)
{
    if (!route || !output)
    {
        return STYX_CONTROL_END;
    }
    pthread_mutex_lock(&route->lock);
    int status = route->reason ? STYX_CONTROL_END : STYX_CONTROL_EMPTY; /* Protected readiness. */
    if (route->ui.count)
    {
        unsigned long tag; /* Private UI record metadata, not exposed as application authority. */
        size_t size; /* Decoded message extent fixed at admission. */
        status = routemailboxnext(route, &route->ui, route->reason, &tag, output, sizeof(*output), &size);
    }
    pthread_mutex_unlock(&route->lock);
    return status;
}


/* EN/RU: See route.h; a too-small destination does not expose tag or consume the head.
 * См. route.h; малый буфер не выдаёт tag и не удаляет первую запись.
 */
int styxbidirroutenext(StyxBidirRoute *route, unsigned long *tag, void *bytes, size_t capacity, size_t *size)
{
    if (!route || !tag || !size || (!bytes && capacity))
    {
        return STYX_CONTROL_END;
    }
    pthread_mutex_lock(&route->lock);
    int status = routemailboxnext(route, &route->general, route->reason, tag, bytes, capacity, size);
    pthread_mutex_unlock(&route->lock);
    return status;
}


/* EN/RU: See internal.h; spurious wakeups never renew the original timeout budget.
 * См. internal.h; ложное пробуждение не продлевает первоначальный timeout.
 */
int routewaitcount(StyxBidirRoute *route, size_t *count, unsigned timeout)
{
    if (!route || !count || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* One monotonic deadline for all wakeups. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&route->lock);
    while (!*count && !route->reason && timeout)
    {
        if (pthread_cond_timedwait(&route->changed, &route->lock, &deadline))
        {
            break;
        }
    }
    int ready = *count != 0; /* Already accepted items remain ready after stop. */
    pthread_mutex_unlock(&route->lock);
    return ready;
}


/* EN/RU: See route.h; select UI/general readiness without consuming any message.
 * См. route.h; выбрать UI/general readiness без потребления сообщений.
 */
int styxbidirroutewait(StyxBidirRoute *route, int ui, unsigned timeout)
{
    if (!route || (ui != 0 && ui != 1))
    {
        return 0;
    }
    return routewaitcount(route, ui ? &route->ui.count : &route->general.count, timeout);
}


/* EN/RU: See route.h; RX binding discovery is deliberately not a source of outgoing authority.
 * См. route.h; обнаружение RX-привязок не даёт исходящих прав.
 */
int styxbidirrouteuisend(StyxBidirRoute *route, unsigned long tag, const DVM_UIMessage *message)
{
    return route && !styxbidirroutestate(route) && styxbidiruisend(route->control, tag, message);
}


/* EN/RU: See route.h; no RX lookup or automatic replay on this independently bound direction.
 * См. route.h; исходящая запись не использует RX lookup и не повторяется автоматически.
 */
int styxbidirroutesend(StyxBidirRoute *route, unsigned long tag, const void *bytes, size_t size)
{
    return route && !styxbidirroutestate(route) && styxbidircontrolsend(route->control, tag, bytes, size);
}


/* EN/RU: See route.h; preserve the underlying transport's bounded TX completion semantics.
 * См. route.h; сохранить ограниченное ожидание завершения TX.
 */
int styxbidirrouteflush(StyxBidirRoute *route, unsigned timeout)
{
    return route && styxbidircontrolflush(route->control, timeout);
}


/* EN/RU: See route.h; expose first route/transport failure independently of consumer progress.
 * См. route.h; показать первую ошибку route/transport независимо от consumer, не потребляя записи.
 */
int styxbidirroutestate(StyxBidirRoute *route)
{
    if (!route)
    {
        return STYX_CONTROL_READ_FAILED;
    }
    pthread_mutex_lock(&route->lock);
    int reason = route->reason; /* Protected first outcome. */
    pthread_mutex_unlock(&route->lock);
    return reason ? reason : styxbidircontrolstate(route->control);
}


/* EN/RU: See route.h; configuration was published before worker creation and never changes.
 * См. route.h; настройка опубликована до запуска потока и больше не меняется.
 */
unsigned styxbidirroutelimit(StyxBidirRoute *route)
{
    return route ? route->limit : 0;
}


/* EN/RU: See route.h; trusted adapters may verify connection identity without opening another session.
 * См. route.h; trusted adapters проверяют identity соединения без открытия другой сессии.
 */
StyxClient *styxbidirrouteclient(StyxBidirRoute *route)
{
    return route ? styxbidircontrolclient(route->control) : NULL;
}


/* EN/RU: See route.h; owned transport retires on routing failures and explicit stop as well.
 * См. route.h; свой transport завершается также при routing failure и explicit stop.
 */
struct DVM_CancelTap *styxbidirroutewatch(StyxBidirRoute *route, void (*receive)(void *),
    void *state, void (*release)(void *))
{
    return route ? styxbidircontrolwatch(route->control, receive, state, release) : NULL;
}


/* EN/RU: See route.h; preserve the underlying immutable grant and refuse a retired dispatcher.
 * См. route.h; сохранить immutable grant и отказать после retirement dispatcher.
 */
StyxFid *styxbidirroutewalk(StyxBidirRoute *route, const char *relative)
{
    return route && !styxbidirroutestate(route) ? styxbidircontrolwalk(route->control, relative) : NULL;
}
