/* Copyright (c) 2026 bhgv
 * EN: Original libstyx scoped resource callbacks retain independent actual VFS opens per fid.
 * RU: Original libstyx scoped resource callbacks удерживают независимые actual VFS opens каждого fid.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "file.h"


/* EN/RU: Owned backing of the explicitly granted service/NULL; caller consumes its wrapper reference.
 * Свой backing явно выданного service/NULL; caller поглощает wrapper reference.
 */
extern StyxBacking *terminalgrantbacking(void);


/* EN/RU: Open exactly the known scoped service Qid and transfer one native open into original fid aux.
 * Открыть только known scoped service Qid и передать native open в original fid aux.
 */
char *terminalgrantresourceopen(Styxserver *server, Qid *qid, int mode)
{
    if (qid->path != 221)
    {
        return Eperm;
    }
    StyxBacking *backing = terminalgrantbacking(); /* Owned backing only while creating this independent open. */
    StyxOpen *opened = backing ? styxfileopen(backing, mode) : nil; /* Consumed by fid cleanup on success. */
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


/* EN/RU: Preserve confirmed original byte prefixes; unknown effects never become fabricated success.
 * Сохранить confirmed original byte prefixes; unknown effects не становятся выдуманным успехом.
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


/* EN/RU: Read current original fid state only; output borrowed, no shared resource cursor.
 * Прочитать только current original fid state; output borrowed, без shared cursor.
 */
char *terminalgrantresourceread(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer transfer; /* Actual per-open VFS outcome. */
    styxfileread(styxgetaux(server), offset, bytes, *count, &transfer);
    return result(transfer, count);
}


/* EN/RU: Write current original fid state only, keeping request framing within the scoped service.
 * Записать только current original fid state, request framing остаётся в scoped service.
 */
char *terminalgrantresourcewrite(Styxserver *server, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer transfer; /* Actual per-open VFS acknowledgement. */
    styxfilewrite(styxgetaux(server), offset, bytes, *count, &transfer);
    return result(transfer, count);
}
