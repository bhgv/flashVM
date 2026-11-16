/* Copyright (c) 2026 bhgv
 * EN: Independent remote fid walking/opening and retained connection ownership.
 * RU: Независимые remote fid walk/open и удержание соединения.
 */
#include "internal.h"


/* EN/RU: Validate and split a canonical relative path before any wire effects.
 * Проверить/split canonical relative path до любых wire effects. */
static int components(const char *path, char *copy, char **names)
{
    unsigned count = 0;
    char *p;
    if (!path || !*path || strlen(path) >= STYX_NAME_MAX || *path == '/')
    {
        return -1;
    }
    strcpy(copy, path);
    if (!strcmp(path, "."))
    {
        return 0;
    }
    names[count++] = copy;
    for (p = copy; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            names[count++] = p+1;
        }
    }
    for (unsigned i = 0; i < count; i++)
    {
        if (!*names[i] || !strcmp(names[i], ".") || !strcmp(names[i], ".."))
        {
            return -1;
        }
    }
    return count;
}


/* EN/RU: See client.h; a partial walk is refused and any new remote fid clunked.
 * См. client.h; partial walk отклонить, новый remote fid поглотить clunk. */
StyxFid *styxclientwalk(StyxClient *client, const char *relative)
{
    char copy[STYX_NAME_MAX];
    char *names[STYX_NAME_MAX];
    int count = components(relative, copy, names);
    if (!client || count < 0)
    {
        return NULL;
    }
    StyxFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return NULL;
    }
    pthread_mutex_lock(&client->lock);
    if (client->failed || client->lastfid == NOFID-1 || client->references == (size_t)-1)
    {
        goto failed;
    }
    fid->number = ++client->lastfid;
    fid->qid = client->root;
    unsigned index = 0;
    do
    {
        Fcall request, reply;
        memset(&request, 0, sizeof(request));
        request.type = Twalk;
        request.fid = index ? fid->number : 1;
        request.newfid = fid->number;
        request.nwname = count-index > MAXWELEM ? MAXWELEM : count-index;
        for (unsigned i = 0; i < request.nwname; i++)
        {
            request.wname[i] = names[index+i];
        }
        int status = styxrpc(client, &request, &reply);
        if (status == 1 && reply.nwqid > request.nwname)
        {
            client->failed = 1;
            status = -1;
        }
        if (status != 1 || reply.nwqid != request.nwname)
        {
            styxclunknumber(client, fid->number);
            goto failed;
        }
        if (reply.nwqid)
        {
            fid->qid = reply.wqid[reply.nwqid-1];
        }
        index += request.nwname;
    } while (index < (unsigned)count);
    fid->client = client;
    client->references++;
    pthread_mutex_unlock(&client->lock);
    return fid;
failed:
    pthread_mutex_unlock(&client->lock);
    free(fid);
    return NULL;
}


/* EN/RU: See client.h; preserve open-mode ceiling and per-fid negotiated iounit.
 * См. client.h; сохранить mode ceiling и negotiated iounit для fid. */
int styxclientopen(StyxFid *fid, int mode)
{
    if (!fid || mode < 0 || mode > 2 || fid->opened)
    {
        return 0;
    }
    StyxClient *client = fid->client;
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = Topen;
    request.fid = fid->number;
    request.mode = mode;
    pthread_mutex_lock(&client->lock);
    int success = styxrpc(client, &request, &reply) == 1;
    if (success)
    {
        fid->qid = reply.qid;
        fid->unit = client->msize-IOHDRSZ;
        if (reply.iounit && reply.iounit < fid->unit)
        {
            fid->unit = reply.iounit;
        }
        fid->opened = 1;
        fid->mode = mode;
    }
    pthread_mutex_unlock(&client->lock);
    return success;
}


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
