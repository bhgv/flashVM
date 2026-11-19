/* Copyright (c) 2026 bhgv
 * EN: Transfer one decoded launch record from the terminal dispatcher to provider admission.
 * RU: Передача одной декодированной записи запуска из диспетчера в очередь провайдера.
 */
#include "../internal.h"
#include "internal.h"
#include "launch.h"


/* EN/RU: See launch.h for caller-owned storage, statuses and metadata I/O.
 * См. launch.h: память вызывающего, статусы и чтение метаданных. */
int styxbidirroutelaunchnext(StyxBidirRoute *route, unsigned index,
    const DVM_LaunchSpec *spec, unsigned access, const StyxTerminalPaths *paths,
    DVM_LaunchEndpoint *endpoint, unsigned long tag, void *scratch, size_t capacity,
    size_t *size, DVM_LaunchTicket **ticket)
{
    if (!route || index >= route->typed_count || !spec || spec->arguments || spec->argument_bytes ||
        !paths || !endpoint || !tag || tag > 0xffffffffUL || (!scratch && capacity) ||
        !size || !ticket || *ticket)
    {
        return STYX_ROUTE_LAUNCH_INVALID;
    }
    StyxClient *terminal = route->control->client; /* Same authenticated connection as the mailbox. */
    if (!styxclientretain(terminal))
    {
        return STYX_ROUTE_LAUNCH_INVALID;
    }
    unsigned long received_tag; /* Direction-local tag from the negotiated mailbox. */
    int status = styxbidirroutetypednext(route, index, &received_tag, scratch, capacity, size);
    if (status != STYX_CONTROL_RECORD)
    {
        styxclientfree(terminal);
        return status;
    }
    FVM_SRecord record = {received_tag, scratch, *size, 0}; /* Payload already decoded by transport. */
    *ticket = styxbidirlaunchrecord(terminal, spec, access, paths, endpoint, tag, &record);
    return *ticket ? STYX_CONTROL_RECORD : STYX_ROUTE_LAUNCH_REJECTED;
}
