/* Copyright (c) 2026 bhgv
 * EN: Remote fid retirement and immutable attached-root kind observation.
 * RU: Remote fid retirement и observation immutable attached-root kind.
 */
#include "internal.h"


/* EN/RU: See client.h; serialize clunk, then release the independent connection pin.
 * См. client.h; сериализовать clunk, затем поглотить ссылку на соединение. */
void styxclientclunk(StyxFid *fid)
{
    if (fid)
    {
        StyxClient *client = fid->client;
        pthread_mutex_lock(&client->lock);
        styxclunknumber(client, fid->number);
        pthread_mutex_unlock(&client->lock);
        free(fid);
        styxclientfree(client);
    }
}


/* EN/RU: See client.h; root Qid is immutable after successful connection construction.
 * См. client.h; root Qid immutable после успешного создания connection. */
int styxclientrootdirectory(StyxClient *client)
{
    return !client || (client->root.type & QTAUTH) ? -1 : (client->root.type & QTDIR) != 0;
}
