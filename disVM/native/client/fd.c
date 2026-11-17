/* Copyright (c) 2026 bhgv
 * EN: Explicit POSIX fd-owner construction, native lifetime pins and shutdown.
 * RU: Явное создание POSIX fd-owner, native lifetime pins и shutdown.
 */
#include "internal.h"
#include "drivers/posix/fd.h"
#include <unistd.h>


/* EN/RU: See client.h; native engine consumes connected fd on all outcomes.
 * См. client.h; native engine принимает connected fd при любом исходе. */
StyxClient *styxclientmountfd(int fd, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit)
{
    return styxclientmountphy(phyposixfd(fd, PHY_STREAM|PHY_ORDERED|PHY_RELIABLE),
        msize, user, name, timeout, limit);
}


/* EN/RU: See client.h; caller already holds a live capability through this call.
 * См. client.h; caller уже удерживает live capability на время вызова. */
int styxclientretain(StyxClient *client)
{
    if (!client)
    {
        return 0;
    }
    pthread_mutex_lock(&client->lock);
    int valid = client->references != (size_t)-1;
    if (valid)
    {
        client->references++;
    }
    pthread_mutex_unlock(&client->lock);
    return valid;
}


/* EN/RU: See client.h; explicit shutdown may wait on native I/O, never on guest execution.
 * См. client.h; explicit shutdown может ждать native I/O, но не guest execution. */
void styxclientshutdown(StyxClient *client)
{
    if (!client)
    {
        return;
    }
    if (client->mux)
    {
        styxmuxshutdown(client->mux);
    }
    else
    {
        pthread_mutex_lock(&client->lock);
        client->failed = 1;
        if (!client->transportclosed)
        {
            client->transportclosed = 1;
            client->transport.close(client->transport.state);
        }
        styxclientemit(client, STYX_LINK_CLOSED);
        pthread_mutex_unlock(&client->lock);
    }
}


/* EN/RU: See client.h; native configuration never performs protocol work on caller.
 * См. client.h; configuration не выполняет protocol work на caller. */
int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses)
{
    return client && client->mux ? styxmuxkeepalive(client->mux, idle, reply, misses) : 0;
}


/* EN/RU: See client.h; subscription context owns only explicitly retained native data.
 * См. client.h; context владеет только явно удержанными native data. */
StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *))
{
    return client ? styxlifeobserve(client->life, receive, state, release) : NULL;
}


/* EN/RU: See client.h; copy independent native source state, without transport access.
 * См. client.h; копировать independent native state без transport access. */
int styxclientlink(StyxClient *client, StyxLinkEvent *output)
{
    return client ? styxlifesnapshot(client->life, output) : 0;
}


/* EN/RU: See client.h; native capability snapshot, not a quota reservation.
 * См. client.h; native capability snapshot, не quota reservation. */
int styxclientwatchable(StyxClient *client)
{
    return client && client->mux ? styxmuxwatchable(client->mux) : 0;
}
