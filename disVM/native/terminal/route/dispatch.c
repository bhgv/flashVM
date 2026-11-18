/* Copyright (c) 2026 bhgv
 * EN: Sole native record consumer; validate bindings before routing copied UI values.
 * RU: Единственный native-потребитель записей; проверка bindings до маршрутизации UI.
 */
#include "internal.h"


/* EN/RU: Validate one already verified tag-zero payload with the existing atomic registry codec.
 * Return 1 binding applied/2 delegated service/0 refused; borrowed route/scratch, only the worker mutates incoming bindings.
 * Проверить tag0 штатным атомарным кодеком; 1 bindings применены/2 другая service/0 отказ, bindings меняет только worker.
 */
static int declaration(StyxBidirRoute *route, size_t size)
{
    size_t encoded; /* Complete rewrapped declaration bytes. */
    size_t used; /* Exact bytes consumed by the original binding codec. */
    FVM_SInteger version; /* Existing first canonical integer selects a tag-zero subservice. */
    size_t prefix; /* Only version identification here; handlers own their schemas. */
    if (fvm_sinteger_read(route->scratch, size, &version, &prefix) != FVM_S_OK || version.negative || version.high)
    {
        return 0;
    }
    if (version.low != FVM_SBINDINGS_VERSION)
    {
        return 2;
    }
    return fvm_srecord_write(0, route->scratch, size, route->encoded, route->limit + 32, &encoded) == FVM_S_OK &&
        fvm_sbindings_apply(&route->incoming, route->encoded, encoded, &used) == FVM_S_OK && used == encoded;
}


/* EN/RU: See internal.h; broker blocks for incoming records, never for recipient capacity; no guest dispatch.
 * См. internal.h; broker ждёт входящие records, не capacity адресата; без guest dispatch.
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
        int service = tag ? 0 : declaration(route, size); /* 1 binding handled here, 2 delegated native service. */
        if (!tag && !service)
        {
            routeend(route, STYX_CONTROL_PROTOCOL);
            return NULL;
        }
        if (!tag && service == 2)
        {
            if (!routeenqueue(route, &route->service, tag, route->scratch, size))
            {
                return NULL;
            }
            continue;
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
            routeenqueue(route, ui ? &route->ui : &route->general, tag, ui ? (const void *)&message : route->scratch,
                ui ? sizeof(message) : size); /* Exactly one destination per record. */
        if (!accepted)
        {
            return NULL;
        }
    }
}
