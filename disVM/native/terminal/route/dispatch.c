/* Copyright (c) 2026 bhgv
 * EN: Sole native record consumer; validate bindings before routing copied UI values.
 * RU: Единственный native-потребитель записей; проверка bindings до маршрутизации UI.
 */
#include "internal.h"


/* EN: Copy scratch bytes or message into its bounded mailbox, waiting only on this native owner.
 * route/tag/size describe the worker's record; message selects UI, NULL selects general.
 * Return 1 once accepted, 0 on explicit stop. No queue lock is held across transport I/O.
 * RU: Скопировать запись из scratch либо message в ограниченную очередь, ожидая только
 * своим native-потоком. message выбирает UI, NULL — общую очередь. 1 — принято, 0 — stop.
 */
static int publish(StyxBidirRoute *route, unsigned long tag, size_t size, const DVM_UIMessage *message)
{
    pthread_mutex_lock(&route->lock);
    unsigned *count = message ? &route->ui_count : &route->count; /* Selected protected occupancy. */
    while (!route->reason && *count == route->slots)
    {
        pthread_cond_wait(&route->changed, &route->lock);
    }
    int accepted = !route->reason; /* Publish only while this route is live. */
    if (accepted)
    {
        unsigned first = message ? route->ui_first : route->first; /* Protected oldest entry. */
        unsigned slot = (first + *count) % route->slots; /* Reserved unpublished tail. */
        if (message)
        {
            route->ui[slot] = *message;
        }
        else
        {
            memcpy(route->raw + slot * route->limit, route->scratch, size);
            route->sizes[slot] = size;
            route->tags[slot] = tag;
        }
        (*count)++;
        pthread_cond_broadcast(&route->changed);
    }
    pthread_mutex_unlock(&route->lock);
    return accepted;
}


/* EN/RU: Validate one already verified tag-zero payload with the existing atomic registry codec.
 * Return 1 applied/0 refused; borrowed route/scratch, only the worker mutates incoming bindings.
 * Проверить tag0 штатным атомарным кодеком; 1 применено/0 отказ, bindings меняет только worker.
 */
static int declaration(StyxBidirRoute *route, size_t size)
{
    size_t encoded; /* Complete rewrapped declaration bytes. */
    size_t used; /* Exact bytes consumed by the original binding codec. */
    return fvm_srecord_write(0, route->scratch, size, route->encoded, route->limit + 32, &encoded) == FVM_S_OK &&
        fvm_sbindings_apply(&route->incoming, route->encoded, encoded, &used) == FVM_S_OK && used == encoded;
}


/* EN/RU: See internal.h; transport waits and mailbox capacity waits never dispatch a guest.
 * См. internal.h; ожидания транспорта и места в очереди не исполняют guest-код.
 */
void *routedispatch(void *opaque)
{
    StyxBidirRoute *route = opaque; /* Borrowed until the host joins this worker. */
    for (;;)
    {
        unsigned long tag; /* Current verified record's numeric route. */
        size_t size; /* Current verified payload bytes. */
        int status = styxbidircontrolnext(route->control, &tag, route->scratch, route->limit, &size);
        if (status == STYX_CONTROL_EMPTY)
        {
            styxbidircontrolwait(route->control, 120000);
            continue;
        }
        if (status != STYX_CONTROL_RECORD)
        {
            routeend(route, status == STYX_CONTROL_END ?
                styxbidircontrolstate(route->control) : STYX_CONTROL_PROTOCOL);
            return NULL;
        }
        if (!tag && !declaration(route, size))
        {
            routeend(route, STYX_CONTROL_PROTOCOL);
            return NULL;
        }
        FVM_SBinding binding; /* Copied validated RX identity, never a TX permission. */
        int bound = tag && fvm_sbindings_lookup(&route->incoming, tag, &binding) == FVM_S_OK;
        int ui = bound && route->version &&
            binding.version == route->version && !strcmp(binding.type, route->type);
        DVM_UIMessage message; /* Validated private UI copy, published only on success. */
        if (ui && styxbidiruidecode(tag, tag, route->scratch, size, &message) != STYX_UI_MESSAGE)
        {
            routeend(route, STYX_CONTROL_PROTOCOL);
            return NULL;
        }
        RouteTypedQueue *typed = NULL; /* Exclusive optional opaque type destination. */
        if (bound && !ui)
        {
            for (unsigned i = 0; i < route->typed_count; i++)
            {
                RouteTypedQueue *candidate = &route->typed[i]; /* Immutable local registration. */
                if (binding.version == candidate->version && !strcmp(binding.type, candidate->type))
                {
                    typed = candidate;
                    break;
                }
            }
        }
        int accepted = typed ? routepublishtyped(route, typed, tag, size) :
            publish(route, tag, size, ui ? &message : NULL); /* Exactly one destination per record. */
        if (!accepted)
        {
            return NULL;
        }
    }
}
