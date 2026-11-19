/* Copyright (c) 2026 bhgv
 * EN: Precise remote chunk acknowledgements, mode ceilings and uncertain-effect poison.
 * RU: Точные remote chunk acknowledgements, mode ceilings и uncertain-effect poison.
 */
#include "internal.h"

/* EN/RU: One native data operation under connection lock; retain exact Rread/Rwrite count.
 * Одна native data-операция под lock; сохранить точный Rread/Rwrite count. */


/* [standalone-contract:chunk]
 * EN: chunk: native client io operations and lifetime.
 * RU: chunk: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * writing (int writing): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * watching (int watching): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void chunk(StyxFid *fid,
                  unsigned long long offset,
                  void *bytes,
                  size_t size,
                  int writing,
                  StyxCancel *cancel,
                  int watching,
                  StyxTransfer *result)
{
    if (!result)
    {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->confirmed = 1;
    if (!fid || !fid->opened || fid->poisoned || offset > 0x7fffffffffffffffULL || (!bytes && size) ||
        (writing ? fid->mode == 0 : fid->mode == 1))
    {
        return;
    }
    if (!size)
    {
        result->success = 1;
        return;
    }
    StyxClient *client = fid->client;
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = writing ? Twrite : Tread;
    request.fid = fid->number;
    request.offset = offset;
    request.count = size > fid->unit ? fid->unit : size;
    request.data = bytes;
    pthread_mutex_lock(&client->lock);
    int status =
        watching ? styxrpcwatch(client, &request, &reply, cancel) : styxrpccancel(client, &request, &reply, cancel);
    result->cancelled = client->cancelled;
    if (status == 1 && reply.count > request.count)
    {
        styxrpcbad(client);
        status = -1;
    }
    if (status == 1)
    {
        result->count = reply.count;
        result->success = 1;
        if (!writing && reply.count)
        {
            memcpy(bytes, reply.data, reply.count);
        }
    }
    else if (client->attempted)
    {
        result->uncertain = 1;
        fid->poisoned = 1;
    }
    pthread_mutex_unlock(&client->lock);
}

/* EN/RU: See client.h; no metadata preflight or read replay.
 * См. client.h; без metadata preflight и read replay. */


/* [standalone-contract:styxclientread]
 * EN: styxclientread: native client io operations and lifetime.
 * RU: styxclientread: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientread(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result)
{
    chunk(fid, offset, bytes, size, 0, NULL, 0, result);
}

/* EN/RU: See client.h; Rerror cannot prove that a write had zero effects.
 * См. client.h; Rerror не доказывает отсутствие write effects. */


/* [standalone-contract:styxclientwrite]
 * EN: styxclientwrite: native client io operations and lifetime.
 * RU: styxclientwrite: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientwrite(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxTransfer *result)
{
    chunk(fid, offset, (void *)bytes, size, 1, NULL, 0, result);
}

/* EN/RU: See client.h; explicit native cancellation is accepted only by fd owner mode.
 * См. client.h; explicit cancellation принимается только fd owner mode. */


/* [standalone-contract:styxclientreadcancel]
 * EN: styxclientreadcancel: native client io operations and lifetime.
 * RU: styxclientreadcancel: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientreadcancel(
    StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result)
{
    if (fid && fid->client->mux)
    {
        chunk(fid, offset, bytes, size, 0, cancel, 0, result);
    }
    else if (result)
    {
        memset(result, 0, sizeof(*result));
        result->confirmed = 1;
    }
}

/* EN/RU: See client.h; preserve original write acknowledgement if it wins the flush race.
 * См. client.h; сохранить original write acknowledgement при победе над flush. */


/* [standalone-contract:styxclientwritecancel]
 * EN: styxclientwritecancel: native client io operations and lifetime.
 * RU: styxclientwritecancel: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientwritecancel(
    StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result)
{
    if (fid && fid->client->mux)
    {
        chunk(fid, offset, (void *)bytes, size, 1, cancel, 0, result);
    }
    else if (result)
    {
        memset(result, 0, sizeof(*result));
        result->confirmed = 1;
    }
}

/* EN/RU: See client.h; explicit cancel is mandatory, fd-owner keeps remaining RPC capacity.
 * См. client.h; explicit cancel обязателен, fd-owner сохраняет оставшуюся RPC capacity. */


/* [standalone-contract:styxclientreadwatch]
 * EN: styxclientreadwatch: native client io operations and lifetime.
 * RU: styxclientreadwatch: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientreadwatch(
    StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result)
{
    if (fid && cancel && styxclientwatchable(fid->client))
    {
        chunk(fid, offset, bytes, size, 0, cancel, 1, result);
    }
    else if (result)
    {
        memset(result, 0, sizeof(*result));
        result->confirmed = 1;
    }
}
