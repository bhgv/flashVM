/* Copyright (c) 2026 bhgv
 * EN: Native Topen and Tcreate with confirmed acknowledgements and uncertain-fid fencing.
 * RU: Нативные Topen и Tcreate с подтверждениями и блокировкой fid при неопределённости.
 */
#include "internal.h"


/* EN: Open fid or create name in that directory with the requested permission bits.
 * mode is the open access. Optional cancel applies to the request. Fill result
 * and return one on acknowledgement. Serialize use and clunk of the borrowed fid.
 * RU: Открыть fid либо создать в каталоге name с переданными правами.
 * mode задаёт доступ. Необязательный cancel отменяет запрос. Заполнить result
 * и вернуть единицу при подтверждении. Сериализовать работу с fid и его clunk. */
static int openfid(StyxFid *fid, const char *name, unsigned permissions, int mode,
    StyxCancel *cancel, StyxTransfer *result)
{
    styxopclear(result);
    if (!fid || mode < 0 || mode > 2 || fid->opened || fid->poisoned)
    {
        return 0;
    }
    StyxClient *client = fid->client;
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = name ? Tcreate : Topen;
    request.name = (char *)name;
    request.perm = permissions;
    request.fid = fid->number;
    request.mode = mode;
    pthread_mutex_lock(&client->lock);
    int status = styxrpccancel(client, &request, &reply, cancel);
    styxopresult(client, status, result);
    int success = result->success;
    if (result->uncertain)
    {
        fid->poisoned = 1;
    }
    if (success)
    {
        fid->qid = reply.qid;
        fid->unit = client->msize - IOHDRSZ;
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


/* EN/RU: See client.h; legacy API now also fences an unacknowledged sent Topen.
 * См. client.h; legacy API тоже блокирует повтор после sent Topen без acknowledgement. */
int styxclientopen(StyxFid *fid, int mode)
{
    StyxTransfer result;
    return openfid(fid, NULL, 0, mode, NULL, &result);
}


/* EN/RU: See client.h; refuse legacy clients or missing result before transport effects.
 * См. client.h; legacy clients/missing result отклонить до transport effects. */
int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result)
{
    if (!result)
    {
        return 0;
    }
    styxopclear(result);
    return fid && fid->client->mux ? openfid(fid, NULL, 0, mode, cancel, result) : 0;
}


/* EN: Validate the borrowed directory and name, then send one create. kind is
 * zero or DMDIR. See openfid for permissions, mode, cancel and result.
 * RU: Проверить каталог и имя, затем послать один create. kind — ноль или DMDIR.
 * Права, mode, cancel и result описаны у openfid. */
static int create_child(StyxFid *directory, const char *name, unsigned permissions, unsigned kind,
    int mode, StyxCancel *cancel, StyxTransfer *result)
{
    if (!result)
    {
        return 0;
    }
    styxopclear(result);
    if (!directory || (cancel && !directory->client->mux) || !(directory->qid.type & QTDIR) || !name || !*name ||
        strlen(name) >= STYX_NAME_MAX || strchr(name, '/') ||
        !strcmp(name, ".") || !strcmp(name, "..") || (permissions & ~0777U))
    {
        return 0;
    }
    return openfid(directory, name, permissions | kind, mode, cancel, result);
}


/* EN/RU: See client.h. / См. client.h. */
int styxclientcreate(StyxFid *directory, const char *name, unsigned permissions, int mode, StyxTransfer *result)
{
    return create_child(directory, name, permissions, 0, mode, NULL, result);
}


/* EN/RU: See client.h. / См. client.h. */
int styxclientmkdir(StyxFid *directory, const char *name, unsigned permissions, StyxTransfer *result)
{
    return create_child(directory, name, permissions, (unsigned)DMDIR, OREAD, NULL, result);
}


/* EN/RU: See client.h. / См. client.h. */
int styxclientcreatecancel(StyxFid *directory, const char *name, unsigned permissions, int mode,
    StyxCancel *cancel, StyxTransfer *result)
{
    return create_child(directory, name, permissions, 0, mode, cancel, result);
}


/* EN/RU: See client.h. / См. client.h. */
int styxclientmkdircancel(StyxFid *directory, const char *name, unsigned permissions,
    StyxCancel *cancel, StyxTransfer *result)
{
    return create_child(directory, name, permissions, (unsigned)DMDIR, OREAD, cancel, result);
}
