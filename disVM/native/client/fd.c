/* Copyright (c) 2026 bhgv
 * EN: Explicit POSIX fd-owner construction, native lifetime pins and shutdown.
 * RU: Явное создание POSIX fd-owner, native lifetime pins и shutdown.
 */
#include "internal.h"
#include <unistd.h>


/* EN/RU: See client.h; native engine consumes connected fd on all outcomes.
 * См. client.h; native engine принимает connected fd при любом исходе. */
StyxClient *styxclientmountfd(int fd, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit)
{
    StyxMux *mux = styxmuxcreate(fd, msize, timeout, limit);
    return mux ? styxclientstart(NULL, mux, msize, user, name) : NULL;
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
        pthread_mutex_unlock(&client->lock);
    }
}
