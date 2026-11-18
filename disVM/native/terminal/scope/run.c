/* Copyright (c) 2026 bhgv
 * EN: Sole native outer-record reader for the explicitly selected nested-envelope broker.
 * RU: Единственный native outer-record reader явного nested-envelope broker.
 */
#include "internal.h"


/* EN/RU: See internal.h; one native reader feeds complete verified outer records, independent inbox consumers.
 * См. internal.h; один native reader принимает outer records, consumers inboxes независимы.
 */
void *scoperun(void *state)
{
    ScopeHub *hub = state; /* Borrowed root owner through join. */
    unsigned char *bytes = hub->scratch; /* Borrowed broker-only storage until worker join. */
    for (;;)
    {
        unsigned long tag; /* Verified transport-local routing tag. */
        size_t size; /* Exact outer payload extent. */
        int status = styxbidircontrolnext(hub->control, &tag, bytes, hub->limit, &size);
        if (status == STYX_CONTROL_EMPTY)
        {
            styxbidircontrolwait(hub->control, 120000);
        }
        else if (status != STYX_CONTROL_RECORD)
        {
            scopeend(hub, status == STYX_CONTROL_END ? styxbidircontrolstate(hub->control) : STYX_CONTROL_PROTOCOL);
            break;
        }
        else if (!scopefeed(hub, tag, bytes, size))
        {
            break;
        }
    }
    return NULL;
}
