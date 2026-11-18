/* Copyright (c) 2026 bhgv
 * EN: Original Styx exports colliding ambient/scoped endpoints and live renamed/repermissioned grants.
 * RU: Original Styx экспортирует одноимённые ambient/scoped endpoints и live rename/permission grants.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "flashvm/sstream.h"
#ifdef STYX_GRANT_RESOURCE_TEST
#include "announcement.h"


/* EN/RU: Scoped original-fid resource open, retaining actual VFS service ownership.
 * Scoped original-fid resource open с actual VFS service ownership.
 */
extern char *terminalgrantresourceopen(Styxserver *, Qid *, int);


/* EN/RU: Scoped original-fid resource byte read, output borrowed during callback.
 * Scoped original-fid resource byte read, output borrowed до return callback.
 */
extern char *terminalgrantresourceread(Styxserver *, char *, ulong *, vlong);


/* EN/RU: Scoped original-fid resource byte write, exact prefix acknowledgement.
 * Scoped original-fid resource byte write с exact prefix acknowledgement.
 */
extern char *terminalgrantresourcewrite(Styxserver *, char *, ulong *, vlong);
#endif


/* EN/RU: Original server owner alone mutates framing/pending reads; atomic counters are test observations.
 * Только original server owner меняет framing/pending reads; atomic counters доступны тесту.
 */
typedef struct Channel
{
    unsigned char offered[256];  /* Fixed bounded framed input and receipt. */
    size_t size;                 /* Encoded output length, owner only. */
    unsigned char incoming[128]; /* Exact fragmented TX decoder storage. */
    FVM_SStream decoder;         /* Owner-only streaming decoder. */
    size_t position;             /* Confirmed TX offset. */
    StyxRead *pending;           /* Owned original deferred idle Tread. */
    unsigned opens;              /* Atomic native file opens. */
    unsigned reads;              /* Atomic native read callbacks. */
    unsigned writes;             /* Atomic native write callbacks. */
} Channel;
static Channel channels[5]; /* Ambient/scoped/nested/replacement/readonly endpoints. */
static const char *labels[] = {"ambient", "scoped", "nested", "replacement", "readonly"}; /* Independent source identity oracles. */


/* EN/RU: Resolve only explicit fixture Qids; no pathname fallback or VFS authority inference.
 * Разрешить только явные fixture Qids, без pathname fallback и вывода VFS authority.
 */
static int indexof(Path path)
{
    switch (path)
    {
    case 7:
        return 0;
    case 201:
    case 203:
    case 204:
        return 1;
    case 211:
        return 2;
    case 401:
        return 3;
    case 202:
        return 4;
    default:
        return -1;
    }
}


/* EN/RU: Append one complete S-record in owner storage; fatal on fixture overflow.
 * Добавить одну полную S-record в owner storage, fatal при overflow fixture.
 */
static void append(Channel *channel, unsigned long tag, const char *text)
{
    size_t encoded; /* Exact complete record size. */
    if (fvm_srecord_write(tag, text, strlen(text), channel->offered + channel->size,
        sizeof(channel->offered) - channel->size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    channel->size += encoded;
}


/* EN/RU: One-time owner initialization, no transport operations or guest callbacks.
 * Однократный owner initialization, без transport operations либо guest callbacks.
 */
void terminalgrantsetup(Styxserver *server)
{
    if (!styxadddir(server, Qroot, 200, "grant", 0555, "fixture") ||
        !styxaddfile(server, 200, 201, "records", 0666, "fixture") ||
        !styxaddfile(server, 200, 202, "readonly", 0444, "fixture") ||
        !styxaddfile(server, 200, 203, "input", 0444, "fixture") ||
        !styxaddfile(server, 200, 204, "output", 0222, "fixture") ||
        !styxadddir(server, 200, 210, "nested", 0555, "fixture") ||
        !styxaddfile(server, 210, 211, "records", 0666, "fixture") ||
        !styxaddfile(server, Qroot, 250, "grantctl", 0222, "fixture"))
    {
        exit(3);
    }
#ifdef STYX_GRANT_RESOURCE_TEST
    if (!styxadddir(server, 200, 220, "movie", 0555, "fixture") ||
        !styxaddfile(server, 220, 221, "1", 0666, "fixture"))
    {
        exit(3);
    }
#endif
    for (unsigned i = 0; i < sizeof(channels) / sizeof(channels[0]); i++)
    {
        append(&channels[i], 41, labels[i]);
        if (fvm_sstream_init(&channels[i].decoder, channels[i].incoming,
            sizeof(channels[i].incoming), sizeof(channels[i].incoming)) != FVM_S_OK)
        {
            exit(3);
        }
    }
#ifdef STYX_GRANT_RESOURCE_TEST
    Channel *job = &channels[1]; /* Existing original offer schema on the explicitly granted job channel. */
    size_t encoded; /* Exact complete S-record extent. */
    if (!movie_offer_binding(23, job->offered + job->size, sizeof(job->offered) - job->size, &encoded))
    {
        exit(3);
    }
    job->size += encoded;
    if (!movie_offer_record(23, MOVIE_OFFER_READY, 1, job->offered + job->size,
        sizeof(job->offered) - job->size, &encoded))
    {
        exit(3);
    }
    job->size += encoded;
#endif
}


/* EN/RU: Open known endpoints only; original server rwx checks precede this callback.
 * Открыть только known endpoints; original server rwx checks выполняются до callback.
 */
char *terminalgrantopen(Styxserver *server, Qid *qid, int mode)
{
#ifdef STYX_GRANT_RESOURCE_TEST
    if (qid->path == 221)
    {
        return terminalgrantresourceopen(server, qid, mode);
    }
#endif
    USED(server);
    if (qid->path == 250)
    {
        return mode == OWRITE ? nil : Eperm;
    }
    int index = indexof(qid->path); /* Explicit fixture channel identity. */
    if (index < 0 || mode < OREAD || mode > ORDWR)
    {
        return Eperm;
    }
    __atomic_add_fetch(&channels[index].opens, 1, __ATOMIC_RELEASE);
    return nil;
}


/* EN/RU: Original owner releases one pending idle read on Tflush/clunk without touching another channel.
 * Original owner освобождает один pending idle read при Tflush/clunk, не трогая другой channel.
 */
static void released(void *state)
{
    Channel *channel = state; /* Static native owner state. */
    StyxRead *read = channel->pending; /* Consume exactly one original handle. */
    channel->pending = nil;
    styxreadfree(read);
}


/* EN/RU: Read exact three-byte prefixes or defer idle Tread, retaining independent endpoint identity.
 * Прочитать точные prefixes по три байта либо defer idle Tread с независимой endpoint identity.
 */
char *terminalgrantread(Styxserver *server, Qid qid, char *bytes, ulong *count, vlong offset)
{
#ifdef STYX_GRANT_RESOURCE_TEST
    if (qid.path == 221)
    {
        return terminalgrantresourceread(server, bytes, count, offset);
    }
#endif
    int index = indexof(qid.path); /* Selected original endpoint. */
    if (index < 0 || offset < 0)
    {
        return Ebadarg;
    }
    Channel *channel = &channels[index]; /* Static server-owner storage. */
    __atomic_add_fetch(&channel->reads, 1, __ATOMIC_RELEASE);
    if ((uvlong)offset > channel->size)
    {
        return Ebadarg;
    }
    if ((uvlong)offset == channel->size)
    {
        if (channel->pending)
        {
            return Eperm;
        }
        channel->pending = styxdeferread(server, channel, released);
        *count = 0;
        return channel->pending ? nil : Enomem;
    }
    size_t available = channel->size - (size_t)offset; /* Exact confirmed prefix. */
    if (*count > available)
    {
        *count = available;
    }
    if (*count > 3)
    {
        *count = 3;
    }
    memmove(bytes, channel->offered + offset, *count);
    return nil;
}


/* EN/RU: Live server-only mutation after capture; keep old directory Qid present under a different name.
 * Live server-only mutation после capture; сохранить old directory Qid под другим именем.
 */
static char *command(Styxserver *server, char *bytes, ulong count)
{
    Styxfile *file = styxfindfile(server, 200); /* Actual captured directory, still server-owned. */
    if (count == 6 && !memcmp(bytes, "rebind", 6))
    {
        free(file->d.name);
        file->d.name = strdup("moved");
        if (!file->d.name || !styxadddir(server, Qroot, 400, "grant", 0555, "fixture") ||
            !styxaddfile(server, 400, 401, "records", 0666, "fixture"))
        {
            exit(3);
        }
        return nil;
    }
    file = styxfindfile(server, 201);
    if (count == 8 && !memcmp(bytes, "readonly", 8))
    {
        file->d.mode = 0444;
        return nil;
    }
    if (count == 8 && !memcmp(bytes, "writable", 8))
    {
        file->d.mode = 0666;
        return nil;
    }
    return Ebadarg;
}


/* EN/RU: Precise two-byte prefix writes, one checked reply produces one receipt on the same scoped channel.
 * Точные prefixes записи по два байта, checked reply даёт одну receipt в том же scoped channel.
 */
char *terminalgrantwrite(Styxserver *server, Qid qid, char *bytes, ulong *count, vlong offset)
{
#ifdef STYX_GRANT_RESOURCE_TEST
    if (qid.path == 221)
    {
        return terminalgrantresourcewrite(server, bytes, count, offset);
    }
#endif
    if (qid.path == 250)
    {
        return offset == 0 ? command(server, bytes, *count) : Ebadarg;
    }
    int index = indexof(qid.path); /* Explicit endpoint identity. */
    if (index < 0 || offset < 0)
    {
        return Ebadarg;
    }
    Channel *channel = &channels[index]; /* Original owner-only framing state. */
    __atomic_add_fetch(&channel->writes, 1, __ATOMIC_RELEASE);
    if ((uvlong)offset != channel->position)
    {
        return Ebadarg;
    }
    if (*count > 2)
    {
        *count = 2;
    }
    size_t at = 0; /* Exact acknowledged prefix in this callback. */
    while (at < *count)
    {
        FVM_SRecord record; /* Checked complete payload borrowed until reset. */
        size_t used = 0; /* Exact decoder consumption. */
        FVM_SResult status = fvm_sstream_push(&channel->decoder, bytes + at, *count - at, &record, &used);
        at += used;
        if (status == FVM_S_OK)
        {
            if (record.tag != 42 || record.size != 5 || memcmp(record.payload, "reply", 5))
            {
                return Ebadarg;
            }
            append(channel, 43, labels[index]);
            if (channel->pending)
            {
                vlong start = styxreadoffset(channel->pending); /* Pending original RX offset. */
                ulong available = channel->size - (size_t)start; /* New receipt prefix. */
                if (available > 3)
                {
                    available = 3;
                }
                if (!styxreadreply(channel->pending, channel->offered + start, available, nil))
                {
                    exit(3);
                }
            }
            fvm_sstream_reset(&channel->decoder);
        }
        else if (status != FVM_S_MORE || !used)
        {
            return Ebadarg;
        }
    }
    channel->position += *count;
    return nil;
}


/* EN/RU: Atomic counters across native server/host owners; invalid index reports zero, no I/O.
 * Atomic counters между server/host owners; invalid index даёт zero, без I/O.
 */
unsigned terminalgrantcalls(unsigned index)
{
    return index < sizeof(channels) / sizeof(channels[0]) ?
        __atomic_load_n(&channels[index].opens, __ATOMIC_ACQUIRE) +
        __atomic_load_n(&channels[index].reads, __ATOMIC_ACQUIRE) +
        __atomic_load_n(&channels[index].writes, __ATOMIC_ACQUIRE) : 0;
}
