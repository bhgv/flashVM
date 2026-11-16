/* Copyright (c) 2026 bhgv
 * EN: Original Inferno stat decoder with bounded owned strings and scalar metadata.
 * RU: Исходный Inferno stat decoder с ограниченными строками и scalar metadata.
 */
#include "internal.h"


/* EN/RU: See client.h; decode only a complete first record, without output on failure.
 * См. client.h; разобрать полную первую запись, при ошибке output не менять. */
int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed)
{
    const uchar *record = bytes;
    if (!bytes || !output || !consumed || size < BIT16SZ)
    {
        return 0;
    }
    unsigned length = BIT16SZ+GBIT16(record);
    if (length > size || statcheck((uchar *)record, length) < 0)
    {
        return 0;
    }
    char *strings = malloc(length+4);
    if (!strings)
    {
        return 0;
    }
    Dir dir;
    int valid = convM2D((uchar *)record, length, &dir, strings) == length;
    if (valid)
    {
        size_t name = strlen(dir.name);
        valid = name < sizeof(output->name) && name == (unsigned)GBIT16(record+STATFIXLEN-4*BIT16SZ);
    }
    if (valid)
    {
        memset(output, 0, sizeof(*output));
        strcpy(output->name, dir.name);
        output->identity = dir.qid.path;
        output->device = dir.dev;
        output->length = dir.length;
        output->directory = (dir.qid.type & QTDIR) != 0;
        output->mode = dir.mode & 0777;
        *consumed = length;
    }
    free(strings);
    return valid;
}


/* EN/RU: See client.h; Tstat addresses the held remote fid, not a reopened path.
 * См. client.h; Tstat обращается к удержанному remote fid, не переоткрывает путь. */
int styxclientstat(StyxFid *fid, StyxMeta *output)
{
    if (!fid || !output)
    {
        return 0;
    }
    StyxClient *client = fid->client;
    Fcall request, reply;
    size_t consumed;
    memset(&request, 0, sizeof(request));
    request.type = Tstat;
    request.fid = fid->number;
    pthread_mutex_lock(&client->lock);
    StyxMeta current;
    int success = styxrpc(client, &request, &reply) == 1 &&
        styxclientrecord(reply.stat, reply.nstat, &current, &consumed) && consumed == reply.nstat;
    if (success)
    {
        *output = current;
    }
    pthread_mutex_unlock(&client->lock);
    return success;
}
