/* Copyright (c) 2026 bhgv
 * EN: Bounded single-flight native session using original Inferno frame codecs.
 * RU: Ограниченная single-flight native session с исходными frame-кодеками Inferno.
 */
#include "internal.h"


/* EN/RU: Transfer exactly n bytes with bounded callback counts; no replay on error.
 * Передать ровно n bytes с проверкой callback counts; без replay при ошибке. */
static int transfer(StyxClient *client, void *bytes, size_t n, int writing)
{
    size_t done = 0;
    while (done < n)
    {
        long count;
        if (writing)
        {
            client->attempted = 1;
            count = client->transport.write(client->transport.state, (char *)bytes+done, n-done);
        }
        else
        {
            count = client->transport.read(client->transport.state, (char *)bytes+done, n-done);
        }
        if (count <= 0 || (size_t)count > n-done)
        {
            return 0;
        }
        done += count;
    }
    return 1;
}


/* EN/RU: See internal.h; validate negotiated bounds, exact kind/tag before publish.
 * См. internal.h; проверить bounds, точные kind/tag до публикации. */
int styxrpc(StyxClient *client, Fcall *request, Fcall *reply)
{
    unsigned length;
    client->attempted = 0;
    if (client->failed)
    {
        return -1;
    }
    request->tag = request->type == Tversion ? NOTAG : 1;
    length = convS2M(request, client->send, client->msize);
    if (!length || !client->transport.begin(client->transport.state))
    {
        return -1;
    }
    if (!transfer(client, client->send, length, 1) ||
        !transfer(client, client->receive, BIT32SZ, 0))
    {
        goto failed;
    }
    length = GBIT32(client->receive);
    if (length < 7 || length > client->msize ||
        !transfer(client, client->receive+BIT32SZ, length-BIT32SZ, 0) ||
        convM2S(client->receive, length, reply) != length || reply->tag != request->tag ||
        (reply->type != request->type+1 && reply->type != Rerror))
    {
        goto failed;
    }
    return reply->type == Rerror ? 0 : 1;
failed:
    client->failed = 1;
    return -1;
}


/* EN/RU: See internal.h; errors cannot cause a second clunk or data replay.
 * См. internal.h; ошибка не вызывает повторный clunk/data replay. */
void styxclunknumber(StyxClient *client, u32int number)
{
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = Tclunk;
    request.fid = number;
    styxrpc(client, &request, &reply);
}


/* EN/RU: See client.h; no network I/O, only a local terminal-state snapshot.
 * См. client.h; без network I/O, только terminal-state snapshot. */
int styxclientalive(StyxClient *client)
{
    if (!client)
    {
        return 0;
    }
    pthread_mutex_lock(&client->lock);
    int alive = !client->failed;
    pthread_mutex_unlock(&client->lock);
    return alive;
}


/* EN/RU: See client.h; the final reference closes transport without waiting for protocol acknowledgement.
 * См. client.h; последняя ссылка закрывает transport без ожидания protocol acknowledgement. */
void styxclientfree(StyxClient *client)
{
    if (!client)
    {
        return;
    }
    pthread_mutex_lock(&client->lock);
    int last = --client->references == 0;
    pthread_mutex_unlock(&client->lock);
    if (last)
    {
        client->transport.close(client->transport.state);
        pthread_mutex_destroy(&client->lock);
        free(client->send);
        free(client->receive);
        free(client);
    }
}


/* EN/RU: See client.h; copy protocol output before the next frame replaces it.
 * См. client.h; скопировать protocol output до следующего frame. */
StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name)
{
    if (!transport || !transport->read || !transport->write || !transport->close || !transport->begin)
    {
        return NULL;
    }
    if (msize < 256 || msize > 65536 || !user || !name || strlen(user) > 4095 || strlen(name) > 4095)
    {
        transport->close(transport->state);
        return NULL;
    }
    StyxClient *client = calloc(1, sizeof(*client));
    if (!client)
    {
        transport->close(transport->state);
        return NULL;
    }
    client->send = malloc(msize);
    client->receive = malloc(msize);
    if (!client->send || !client->receive || pthread_mutex_init(&client->lock, NULL))
    {
        free(client->send);
        free(client->receive);
        free(client);
        transport->close(transport->state);
        return NULL;
    }
    client->transport = *transport;
    client->msize = msize;
    client->references = 1;
    client->lastfid = 1;
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = Tversion;
    request.version = VERSION9P;
    request.msize = msize;
    if (styxrpc(client, &request, &reply) != 1 || reply.msize < 256 || reply.msize > msize ||
        GBIT32(client->receive) != 7+BIT32SZ+BIT16SZ+6 || strcmp(reply.version, VERSION9P))
    {
        styxclientfree(client);
        return NULL;
    }
    client->msize = reply.msize;
    request.type = Tattach;
    request.fid = 1;
    request.afid = NOFID;
    request.uname = (char *)user;
    request.aname = (char *)name;
    if (styxrpc(client, &request, &reply) != 1)
    {
        styxclientfree(client);
        return NULL;
    }
    client->root = reply.qid;
    return client;
}
