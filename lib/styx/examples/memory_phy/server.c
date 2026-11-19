/* Copyright (c) 2026 bhgv
 * EN: Original libstyx callbacks and filesystem over a custom granted byte carrier.
 * RU: Исходные libstyx callbacks и filesystem поверх custom granted byte carrier.
 * standalone-reviewed: no private codec, network listener or interpreter scheduler.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "demo.h"
#include <server.h>

static const char contents[] = "memory-only provider\n"; /* Immutable file body; no GUI or terminal context. */


/* EN: qid/mode are borrowed open arguments. Permit read-only root/status; return
 * original Rerror text on denial. No allocation or modification of namespace state.
 * RU: qid/mode — параметры открытия. Разрешить readonly root/status; при отказе
 * вернуть исходный Rerror text. Без allocation и изменения namespace.
 */
static char *open_file(Qid *qid, int mode)
{
    return (qid->path == Qroot || qid->path == 1) && mode == OREAD ? nil : Eperm;
}


/* EN: qid selects status; bytes/count are the borrowed output span. offset is a
 * signed 64-bit cursor. Return nil with actual byte count, or original Rerror.
 * RU: qid выбирает status; bytes/count — выходной буфер. offset — signed64 cursor.
 * Вернуть nil с реальным byte count либо исходный Rerror.
 */
static char *read_file(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path != 1 || offset < 0)
    {
        return Ebadarg;
    }
    *count = offset >= (vlong)(sizeof(contents) - 1) ? 0 : styxreadstr(offset, bytes, *count, (char *)contents);
    return nil; /* A large valid offset is EOF, never a truncated native offset. */
}


/* EN/RU: See demo.h; only this native thread ever operates on server/ops/files.
 * См. demo.h; только этот native thread работает с server/ops/files.
 */
void *memory_server(void *state)
{
    DemoServer *owner = state;
    Styxserver server = {0};
    Styxops ops = {0};
    ops.open = open_file;
    ops.read = read_file;
    char *error = styxphyinit(&server, &ops, owner->phy, 0555, 1, 2); /* Consume the granted endpoint even if setup fails. */
    int initialized = error == nil;
    Styxfile *file = initialized ? styxaddfile(&server, Qroot, 1, "status", 0444, "demo") : nil;
    if (file)
    {
        file->d.length = sizeof(contents) - 1; /* Metadata agrees with this provider's actual file contents. */
    }
    pthread_mutex_lock(&owner->lock);
    owner->success = file != nil;
    owner->initialized = 1;
    pthread_cond_signal(&owner->ready);
    pthread_mutex_unlock(&owner->lock);
    if (file)
    {
        while (!styxwait(&server) && !styxprocess(&server))
        {
            /* Each original 9P operation resolves against this server's live file table. */
        }
    }
    owner->clean = !initialized || styxend(&server) == nil; /* Terminal result is read only after join, never as startup state. */
    return NULL;
}
