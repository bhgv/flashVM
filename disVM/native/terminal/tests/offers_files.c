/* Copyright (c) 2026 bhgv
 * EN: Original libstyx per-fid callbacks preserve independent resource open state on one connection.
 * RU: Исходные libstyx per-fid callbacks сохраняют независимые resource opens одного соединения.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "file.h"


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


/* EN/RU: Read current fid only, with its exact offset and native state; no shared service cursor.
 * Прочитать только current fid по exact offset/native state, без общего service cursor.
 */
char *offerfileread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
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
