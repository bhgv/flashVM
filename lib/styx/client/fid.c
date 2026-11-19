/* Copyright (c) 2026 bhgv
 * EN: Remote fid retirement and immutable attached-root kind observation.
 * RU: Remote fid retirement и observation immutable attached-root kind.
 */
#include "internal.h"

/* EN/RU: See client.h; serialize clunk, then release the independent connection pin.
 * См. client.h; сериализовать clunk, затем поглотить ссылку на соединение. */


/* [standalone-contract:styxclientclunk]
 * EN: styxclientclunk: native client fid operations and lifetime.
 * RU: styxclientclunk: нативные client-операции fid и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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

/* EN/RU: See client.h; root Qid is immutable after successful connection construction.
 * См. client.h; root Qid immutable после успешного создания connection. */


/* [standalone-contract:styxclientrootdirectory]
 * EN: styxclientrootdirectory: native client fid operations and lifetime.
 * RU: styxclientrootdirectory: нативные client-операции fid и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientrootdirectory(StyxClient *client)
{
    return !client || (client->root.type & QTAUTH) ? -1 : (client->root.type & QTDIR) != 0;
}

/* EN/RU: See client.h; construction fixes transport kind for the client lifetime.
 * См. client.h; construction фиксирует transport kind до конца client lifetime. */


/* [standalone-contract:styxclientcancellable]
 * EN: styxclientcancellable: native client fid operations and lifetime.
 * RU: styxclientcancellable: нативные client-операции fid и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientcancellable(StyxClient *client)
{
    return client && client->mux != NULL;
}

/* EN/RU: See client.h; borrow pinned fid while observing the walked object's immutable kind.
 * См. client.h; заимствовать pinned fid при чтении immutable kind walked object. */


/* [standalone-contract:styxclientfiddirectory]
 * EN: styxclientfiddirectory: native client fid operations and lifetime.
 * RU: styxclientfiddirectory: нативные client-операции fid и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientfiddirectory(StyxFid *fid)
{
    return !fid || (fid->qid.type & QTAUTH) ? -1 : (fid->qid.type & QTDIR) != 0;
}
