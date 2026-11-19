/* Copyright (c) 2026 bhgv
 * EN: Native Topen with acknowledgement-preserving cancellation and uncertain-fid fencing.
 * RU: Native Topen с acknowledgement-preserving cancellation и uncertain-fid fencing.
 */
#include "internal.h"

/* EN/RU: See client.h; preserve open-mode ceiling and per-fid negotiated iounit.
 * См. client.h; сохранить mode ceiling и negotiated iounit для fid. */


/* [standalone-contract:openfid]
 * EN: openfid: native client open operations and lifetime.
 * RU: openfid: нативные client-операции open и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int openfid(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result)
{
    styxopclear(result);
    if (!fid || mode < 0 || mode > 2 || fid->opened || fid->poisoned)
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


/* [standalone-contract:styxclientopen]
 * EN: styxclientopen: native client open operations and lifetime.
 * RU: styxclientopen: нативные client-операции open и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientopen(StyxFid *fid, int mode)
{
    StyxTransfer result;
    return openfid(fid, mode, NULL, &result);
}

/* EN/RU: See client.h; refuse legacy clients or missing result before transport effects.
 * См. client.h; legacy clients/missing result отклонить до transport effects. */


/* [standalone-contract:styxclientopencancel]
 * EN: styxclientopencancel: native client open operations and lifetime.
 * RU: styxclientopencancel: нативные client-операции open и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result)
{
    if (!result)
    {
        return 0;
    }
    styxopclear(result);
    return fid && fid->client->mux ? openfid(fid, mode, cancel, result) : 0;
}
