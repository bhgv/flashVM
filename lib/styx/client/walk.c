/* Copyright (c) 2026 bhgv
 * EN: Root-relative native walks with logical cancellation across MAXWELEM fragments.
 * RU: Root-relative native walk с logical cancellation через MAXWELEM fragments.
 */
#include "internal.h"

/* EN/RU: Validate and split a canonical relative path before any wire effects.
 * Проверить/split canonical relative path до любых wire effects. */


/* [standalone-contract:components]
 * EN: components: native client walk operations and lifetime.
 * RU: components: нативные client-операции walk и время жизни.
 * path (const char *path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * copy (char *copy): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * names (char **names): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int components(const char *path, char *copy, char **names)
{
    unsigned count = 0;
    char *p;
    if (!path || !*path || strlen(path) >= STYX_NAME_MAX || *path == '/')
    {
        return -1;
    }
    strcpy(copy, path);
    if (!strcmp(path, "."))
    {
        return 0;
    }
    names[count++] = copy;
    for (p = copy; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            names[count++] = p + 1;
        }
    }
    for (unsigned i = 0; i < count; i++)
    {
        if (!*names[i] || !strcmp(names[i], ".") || !strcmp(names[i], ".."))
        {
            return -1;
        }
    }
    return count;
}

/* EN/RU: See client.h; a partial walk is refused and any new remote fid clunked.
 * См. client.h; partial walk отклонить, новый remote fid поглотить clunk. */


/* [standalone-contract:walk]
 * EN: walk: native client walk operations and lifetime.
 * RU: walk: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * asynchronous (int asynchronous): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * missing (int *missing): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static StyxFid *walk(StyxClient *client,
                     StyxFid *base,
                     const char *relative,
                     StyxCancel *cancel,
                     int asynchronous,
                     StyxTransfer *result,
                     int *missing)
{
    styxopclear(result);
    if (missing)
    {
        *missing = 0;
    }
    char copy[STYX_NAME_MAX];
    char *names[STYX_NAME_MAX];
    int count = components(relative, copy, names);
    if (!client || count < 0)
    {
        return NULL;
    }
    StyxFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return NULL;
    }
    if (!styxcancelbegin(cancel))
    {
        free(fid);
        return NULL;
    }
    const char *prefix = base ? base->walk_path : ".";
    size_t first = strcmp(prefix, ".") ? strlen(prefix) : 0;
    size_t tail = strcmp(relative, ".") ? strlen(relative) : 0;
    if (first + tail + 2 > sizeof(fid->walk_path))
    {
        styxcancelend(cancel);
        free(fid);
        return NULL;
    }
    memcpy(fid->walk_path, prefix, first);
    if (first && tail)
    {
        fid->walk_path[first++] = '/';
    }
    memcpy(fid->walk_path + first, relative, tail);
    fid->walk_path[first + tail] = 0;
    if (!first && !tail)
    {
        strcpy(fid->walk_path, ".");
    }
    int remote = 0;
    pthread_mutex_lock(&client->lock);
    if ((base && (base->opened || base->poisoned)) || (count && !((base ? base->qid : client->root).type & QTDIR)) ||
        client->failed || client->lastfid == NOFID - 1 || client->references == (size_t)-1)
    {
        goto failed;
    }
    fid->number = ++client->lastfid;
    fid->qid = base ? base->qid : client->root;
    unsigned index = 0;
    do
    {
        Fcall request, reply;
        memset(&request, 0, sizeof(request));
        request.type = Twalk;
        request.fid = index ? fid->number : base ? base->number : 1;
        request.newfid = fid->number;
        unsigned limit = missing && client->error_profile == STYX_ERRORS_INFERNO ? 1 : MAXWELEM;
        request.nwname = count - index > limit ? limit : count - index;
        for (unsigned i = 0; i < request.nwname; i++)
        {
            request.wname[i] = names[index + i];
        }
        StyxCancel *step = cancel ? styxcancelstep(cancel) : NULL;
        if (cancel && !step)
        {
            goto abandoned;
        }
        int status = styxrpccancel(client, &request, &reply, step);
        remote |= client->attempted;
        if (status == 1 && reply.nwqid > request.nwname)
        {
            styxrpcbad(client);
            status = -1;
        }
        styxopresult(client, status, result);
        if (status != 1 || reply.nwqid != request.nwname)
        {
            if (missing && request.nwname && status == 0 && client->error_profile == STYX_ERRORS_INFERNO &&
                reply.ename && !strcmp(reply.ename, "file does not exist"))
            {
                *missing = 1;
            }
            result->success = 0;
            goto abandoned;
        }
        if (reply.nwqid)
        {
            fid->qid = reply.wqid[reply.nwqid - 1];
        }
        index += request.nwname;
    } while (index < (unsigned)count);
    fid->client = client;
    client->references++;
    pthread_mutex_unlock(&client->lock);
    styxcancelend(cancel);
    return fid;
abandoned:
    result->success = 0;
    if (remote)
    {
        if (asynchronous)
        {
            styxforgetlocked(client, fid->number);
        }
        else
        {
            styxclunknumber(client, fid->number);
        }
    }
failed:
    pthread_mutex_unlock(&client->lock);
    styxcancelend(cancel);
    free(fid);
    return NULL;
}

/* EN/RU: See client.h; legacy walks stay confined to the original attached root.
 * См. client.h; legacy walks остаются внутри исходного attach root. */


/* [standalone-contract:styxclientwalk]
 * EN: styxclientwalk: native client walk operations and lifetime.
 * RU: styxclientwalk: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalk(StyxClient *client, const char *relative)
{
    StyxTransfer result;
    return walk(client, NULL, relative, NULL, 0, &result, NULL);
}

/* EN/RU: See client.h; caller pins unopened base across all fragmented walk RPCs.
 * См. client.h; caller удерживает unopened base на время всех walk RPC fragments. */


/* [standalone-contract:styxclientwalkfrom]
 * EN: styxclientwalkfrom: native client walk operations and lifetime.
 * RU: styxclientwalkfrom: нативные client-операции walk и время жизни.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalkfrom(StyxFid *base, const char *relative)
{
    StyxTransfer result;
    return base ? walk(base->client, base, relative, NULL, 0, &result, NULL) : NULL;
}

/* EN/RU: See client.h; cancellation spans every walk fragment and failure cleanup is detached.
 * См. client.h; cancellation покрывает все walk fragments, failure cleanup detached. */


/* [standalone-contract:styxclientwalkcancel]
 * EN: styxclientwalkcancel: native client walk operations and lifetime.
 * RU: styxclientwalkcancel: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalkcancel(StyxClient *client, const char *relative, StyxCancel *cancel, StyxTransfer *result)
{
    if (!result)
    {
        return NULL;
    }
    styxopclear(result);
    return client && client->mux ? walk(client, NULL, relative, cancel, 1, result, NULL) : NULL;
}

/* EN/RU: See client.h; base stays borrowed/pinned/unopened throughout fragmented native I/O.
 * См. client.h; base заимствован, pinned/unopened на время fragmented native I/O. */


/* [standalone-contract:styxclientwalkfromcancel]
 * EN: styxclientwalkfromcancel: native client walk operations and lifetime.
 * RU: styxclientwalkfromcancel: нативные client-операции walk и время жизни.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalkfromcancel(StyxFid *base, const char *relative, StyxCancel *cancel, StyxTransfer *result)
{
    if (!result)
    {
        return NULL;
    }
    styxopclear(result);
    return base && base->client->mux ? walk(base->client, base, relative, cancel, 1, result, NULL) : NULL;
}

/* EN/RU: See client.h; error profile freezes once any remote fid is allocated or client shared.
 * См. client.h; profile нельзя менять после sharing или allocation remote fid. */


/* [standalone-contract:styxclienterrorprofile]
 * EN: styxclienterrorprofile: native client walk operations and lifetime.
 * RU: styxclienterrorprofile: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * profile (unsigned profile): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclienterrorprofile(StyxClient *client, unsigned profile)
{
    if (!client || profile > STYX_ERRORS_INFERNO)
    {
        return 0;
    }
    pthread_mutex_lock(&client->lock);
    int valid = !client->failed && client->references == 1 && client->lastfid == 1;
    if (valid)
    {
        client->error_profile = profile;
    }
    pthread_mutex_unlock(&client->lock);
    return valid;
}

/* EN/RU: See client.h; preserve lookup-local missing across cleanup and concurrent RPCs.
 * См. client.h; lookup-local missing сохраняется через cleanup и concurrent RPCs. */


/* [standalone-contract:styxclientlookup]
 * EN: styxclientlookup: native client walk operations and lifetime.
 * RU: styxclientlookup: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * path (const char *path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * missing (int *missing): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientlookup(StyxClient *client, const char *path, StyxCancel *cancel, int *missing)
{
    if (!missing)
    {
        return NULL;
    }
    *missing = 0;
    StyxTransfer result;
    return client && (!cancel || client->mux) ? walk(client, NULL, path, cancel, cancel != NULL, &result, missing)
                                              : NULL;
}

/* EN/RU: See client.h; base remains pinned by native caller throughout the logical path.
 * См. client.h; base остаётся pinned caller на время logical path. */


/* [standalone-contract:styxclientlookupfrom]
 * EN: styxclientlookupfrom: native client walk operations and lifetime.
 * RU: styxclientlookupfrom: нативные client-операции walk и время жизни.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * path (const char *path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * missing (int *missing): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientlookupfrom(StyxFid *base, const char *path, StyxCancel *cancel, int *missing)
{
    if (!missing)
    {
        return NULL;
    }
    *missing = 0;
    StyxTransfer result;
    return base && (!cancel || base->client->mux)
               ? walk(base->client, base, path, cancel, cancel != NULL, &result, missing)
               : NULL;
}
