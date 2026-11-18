/* Copyright (c) 2026 bhgv
 * EN: Original libstyx per-fid callbacks preserve independent resource open state on one connection.
 * RU: Исходные libstyx per-fid callbacks сохраняют независимые resource opens одного соединения.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "file.h"
#include "checkpoint.h"
#ifndef STYX_FETCH_CANCEL_CASE
#define STYX_FETCH_CANCEL_CASE 0 /* First actual resource Tread remains pending until Tflush. */
#endif
static StyxRead *pending; /* Original deferred read owned by the original server thread. */
static unsigned deferred; /* Only the first resource read is held; later fids remain independent. */


/* EN/RU: Publish an S-framed checkpoint on this actual requesting client's control stream, server-owner only.
 * S-framed checkpoint в control stream настоящего requesting client, только server-owner.
 */
extern void fetchpendingnotice(void);


/* EN/RU: Owned prepared native backing for a known fixture generation, no arbitrary path authority.
 * Owned prepared native backing известного generation fixture, без прав по произвольному path.
 */
extern StyxBacking *offerbacking(unsigned generation);


/* EN/RU: Attach one independent native open to current original fid; server/qid borrowed, 0 success.
 * Прикрепить отдельный native open к original fid, server/qid заимствованы, 0 — успех.
 */
char *offerfileopen(Styxserver *server, Qid *qid, int mode)
{
    StyxBacking *backing = offerbacking((unsigned)(qid->path - 99)); /* Owned exact generation node. */
    StyxOpen *opened = backing ? styxfileopen(backing, mode) : nil; /* New independent open. */
    styxbackingfree(backing);
    if (!opened)
    {
        return Eperm;
    }
    if (styxsetaux(server, opened))
    {
        styxfileclose(opened);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Preserve original precise prefix acknowledgements without inventing success on failed I/O.
 * Сохранить original precise prefix ACK, не выдумывать success при failed I/O.
 */
static char *result(StyxTransfer transfer, ulong *count)
{
    if (!transfer.confirmed || transfer.uncertain || (!transfer.success && !transfer.count))
    {
        return Eperm;
    }
    *count = transfer.count;
    return nil;
}


/* EN/RU: Original owner consumes one deferred read and reports native Tflush/close completion.
 * Original owner поглощает deferred read и сообщает завершение native Tflush/close.
 */
static void released(void *unused)
{
    USED(unused);
    StyxRead *read = pending; /* Original retained handle, freed exactly once. */
    pending = nil;
    styxreadfree(read);
    fetchmark(FETCH_FLUSH);
}


/* EN/RU: Read current fid only, with its exact offset and native state; no shared service cursor.
 * Прочитать только current fid по exact offset/native state, без общего service cursor.
 */
char *offerfileread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    if (STYX_FETCH_CANCEL_CASE < 6 && !deferred++)
    {
        pending = styxdeferread(server, nil, released);
        if (!pending)
        {
            return Enomem;
        }
        *count = 0;
        fetchmark(FETCH_RPC);
        fetchpendingnotice();
        return nil;
    }
    StyxTransfer transfer; /* Exact original native outcome. */
    styxfileread(styxgetaux(server), offset, bytes, *count, &transfer);
    return result(transfer, count);
}


/* EN/RU: Write one current-fid prefix only, preserving original service framing and acknowledgement.
 * Записать prefix только current fid, сохраняя original framing/ACK.
 */
char *offerfilewrite(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer transfer; /* Exact original native outcome. */
    styxfilewrite(styxgetaux(server), offset, bytes, *count, &transfer);
    return result(transfer, count);
}
