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
static int serialrpc(StyxClient *client, Fcall *request, Fcall *reply)
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
    if (!client->terminal)
    {
        client->terminal = STYX_LINK_FAILED;
    }
    return -1;
}


/* EN/RU: Borrow client/request/reply/cancel under client lock; watching disables ordinary
 * deadline only. Return1 reply,0 Rerror,-1 failure; reply borrows client receive storage.
 * Под client lock заимствовать inputs; watching снимает обычный deadline, но не flush grace.
 * Вернуть1 reply,0 Rerror,-1 failure; reply заимствует client receive storage. */
static int exchange(StyxClient *client, Fcall *request, Fcall *reply, StyxCancel *cancel, int watching)
{
    client->attempted = 0;
    client->cancelled = 0;
    if (client->failed)
    {
        return -1;
    }
    if (!client->mux)
    {
        int status = (cancel || watching) ? -1 : serialrpc(client, request, reply);
        if (client->failed)
        {
            styxclientemit(client, STYX_LINK_FAILED);
        }
        return status;
    }
    if (styxcancelled(cancel))
    {
        client->cancelled = 1;
        return -1;
    }
    unsigned length = convS2M(request, client->send, client->msize);
    StyxFlight *flight = length ? styxmuxsubmit(client->mux, client->send, length, client->msize, cancel, watching) : NULL;
    if (!flight)
    {
        if (!styxmuxalive(client->mux))
        {
            client->failed = 1;
        }
        return -1;
    }
    pthread_mutex_unlock(&client->lock);
    StyxAnswer answer = styxmuxcollect(flight);
    pthread_mutex_lock(&client->lock);
    client->attempted = answer.attempted;
    client->cancelled = answer.cancelled;
    int status = answer.status;
    if (answer.length)
    {
        memcpy(client->receive, answer.bytes, answer.length);
        if (convM2S(client->receive, answer.length, reply) != answer.length)
        {
            styxrpcbad(client);
            status = -1;
        }
    }
    free(answer.bytes);
    if (!styxmuxalive(client->mux))
    {
        client->failed = 1;
    }
    return status;
}


/* EN/RU: See internal.h; ordinary native RPC retains its normal deadline.
 * См. internal.h; ordinary native RPC сохраняет обычный deadline. */
int styxrpccancel(StyxClient *client, Fcall *request, Fcall *reply, StyxCancel *cancel)
{
    return exchange(client, request, reply, cancel, 0);
}


/* EN/RU: See internal.h; long-lived read still uses ordinary tag/flush fences.
 * См. internal.h; long-lived read использует обычные tag/flush fences. */
int styxrpcwatch(StyxClient *client, Fcall *request, Fcall *reply, StyxCancel *cancel)
{
    return exchange(client, request, reply, cancel, 1);
}


/* EN/RU: See internal.h; ordinary RPC uses native multiplexer when explicitly configured.
 * См. internal.h; обычный RPC использует явно настроенный native multiplexer. */
int styxrpc(StyxClient *client, Fcall *request, Fcall *reply)
{
    return styxrpccancel(client, request, reply, NULL);
}


/* EN/RU: See internal.h; a semantic protocol failure also retires other admitted work.
 * См. internal.h; semantic protocol failure завершает другие admitted work. */
void styxrpcbad(StyxClient *client)
{
    client->failed = 1;
    if (client->mux)
    {
        styxmuxfail(client->mux);
    }
    else
    {
        styxclientemit(client, STYX_LINK_FAILED);
    }
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
    int alive = !client->failed && (!client->mux || styxmuxalive(client->mux));
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
        styxlifequiet(client->life);
        if (client->mux)
        {
            styxmuxfree(client->mux);
        }
        else if (!client->transportclosed)
        {
            client->transport.close(client->transport.state);
        }
        styxlifedrop(client->life);
        pthread_mutex_destroy(&client->lock);
        free(client->send);
        free(client->receive);
        free(client);
    }
}


/* EN/RU: Constructor failures consume native inputs, joining only in this explicit I/O path.
 * Constructor failure поглощает native inputs; join только в explicit I/O path. */
static void startfailed(const StyxTransport *transport, StyxMux *mux)
{
    if (mux)
    {
        styxmuxshutdown(mux);
        styxmuxfree(mux);
    }
    else
    {
        transport->close(transport->state);
    }
}


/* EN/RU: See client.h; copy protocol output before the next frame replaces it.
 * См. client.h; скопировать protocol output до следующего frame. */
StyxClient *styxclientstart(const StyxTransport *transport, StyxMux *mux, unsigned msize, const char *user, const char *name)
{
    if (!mux && (!transport || !transport->read || !transport->write || !transport->close || !transport->begin))
    {
        return NULL;
    }
    if (msize < 256 || msize > 65536 || !user || !name || strlen(user) > 4095 || strlen(name) > 4095)
    {
        startfailed(transport, mux);
        return NULL;
    }
    StyxClient *client = calloc(1, sizeof(*client));
    if (!client)
    {
        startfailed(transport, mux);
        return NULL;
    }
    client->send = malloc(msize);
    client->receive = malloc(msize);
    if (!client->send || !client->receive || pthread_mutex_init(&client->lock, NULL))
    {
        free(client->send);
        free(client->receive);
        free(client);
        startfailed(transport, mux);
        return NULL;
    }
    client->life = mux ? styxmuxlife(mux) : styxlifecreate();
    if (!client->life || (mux && !styxliferetain(client->life)))
    {
        pthread_mutex_destroy(&client->lock);
        free(client->send);
        free(client->receive);
        free(client);
        startfailed(transport, mux);
        return NULL;
    }
    client->mux = mux;
    if (transport)
    {
        client->transport = *transport;
    }
    client->msize = msize;
    client->references = 1;
    client->lastfid = 1;
    Fcall request, reply;
    pthread_mutex_lock(&client->lock);
    memset(&request, 0, sizeof(request));
    request.type = Tversion;
    request.version = VERSION9P;
    request.msize = msize;
    if (styxrpc(client, &request, &reply) != 1 || reply.msize < 256 || reply.msize > msize ||
        GBIT32(client->receive) != 7+BIT32SZ+BIT16SZ+6 || strcmp(reply.version, VERSION9P))
    {
        pthread_mutex_unlock(&client->lock);
        styxclientshutdown(client);
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
        pthread_mutex_unlock(&client->lock);
        styxclientshutdown(client);
        styxclientfree(client);
        return NULL;
    }
    client->root = reply.qid;
    pthread_mutex_unlock(&client->lock);
    return client;
}


/* EN/RU: See client.h; legacy transport keeps its existing serialized exchange policy.
 * См. client.h; legacy transport сохраняет serialized exchange policy. */
StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name)
{
    return styxclientstart(transport, NULL, msize, user, name);
}


/* EN/RU: See internal.h; preserve first legacy cause and admit callbacks outside client lock.
 * См. internal.h; сохранить first legacy cause и вызвать callbacks вне client lock. */
void styxclientemit(StyxClient *client, int reason)
{
    if (!client->terminal)
    {
        client->terminal = reason;
    }
    reason = client->terminal;
    pthread_mutex_unlock(&client->lock);
    styxlifeemit(client->life, reason);
    pthread_mutex_lock(&client->lock);
}


/* EN/RU: See client.h; source lifetime is retained by accepted watcher nodes.
 * См. client.h; source lifetime удерживается принятыми watcher nodes. */
StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *))
{
    return client ? styxlifewatch(client->life, notify, state, release) : NULL;
}
