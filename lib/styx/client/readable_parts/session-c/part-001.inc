/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of session.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv
 * EN: Bounded single-flight native session using original Inferno frame codecs.
 * RU: Ограниченная single-flight native session с исходными frame-кодеками Inferno.
 */
#include "internal.h"

/* EN/RU: Transfer exactly n bytes with bounded callback counts; no replay on error.
 * Передать ровно n bytes с проверкой callback counts; без replay при ошибке. */


/* [standalone-contract:transfer]
 * EN: transfer: native client session operations and lifetime.
 * RU: transfer: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * n (size_t n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * writing (int writing): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int transfer(StyxClient *client, void *bytes, size_t n, int writing)
{
    size_t done = 0;
    while (done < n)
    {
        long count;
        if (writing)
        {
            client->attempted = 1;
            count = client->transport.write(client->transport.state, (char *)bytes + done, n - done);
        }
        else
        {
            count = client->transport.read(client->transport.state, (char *)bytes + done, n - done);
        }
        if (count <= 0 || (size_t)count > n - done)
        {
            return 0;
        }
        done += count;
    }
    return 1;
}

/* EN/RU: See internal.h; validate negotiated bounds, exact kind/tag before publish.
 * См. internal.h; проверить bounds, точные kind/tag до публикации. */


/* [standalone-contract:serialrpc]
 * EN: serialrpc: native client session operations and lifetime.
 * RU: serialrpc: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int serialrpc(StyxClient *client, Fcall *request, Fcall *reply)
{
    unsigned length;
    client->attempted = 0;
    if (client->failed)
    {
        return -1;
    }
    request->tag = request->type == Tversion ? NOTAG : 1;
    length = convS2M(request, client->send, client->msize);
    if (!length || !client->transport.begin(client->transport.state))
    {
        return -1;
    }
    if (!transfer(client, client->send, length, 1) || !transfer(client, client->receive, BIT32SZ, 0))
    {
        goto failed;
    }
    length = GBIT32(client->receive);
    if (length < 7 || length > client->msize || !transfer(client, client->receive + BIT32SZ, length - BIT32SZ, 0) ||
        convM2S(client->receive, length, reply) != length || reply->tag != request->tag ||
        (reply->type != request->type + 1 && reply->type != Rerror))
    {
        goto failed;
    }
    return reply->type == Rerror ? 0 : 1;
failed:
    client->failed = 1;
    if (!client->terminal)
    {
        client->terminal = STYX_LINK_FAILED;
    }
    return -1;
}

/* EN/RU: Borrow client/request/reply/cancel under client lock; watching disables ordinary
 * deadline only. Return1 reply,0 Rerror,-1 failure; reply borrows client receive storage.
 * Под client lock заимствовать inputs; watching снимает обычный deadline, но не flush grace.
 * Вернуть1 reply,0 Rerror,-1 failure; reply заимствует client receive storage. */


/* [standalone-contract:exchange]
 * EN: exchange: native client session operations and lifetime.
 * RU: exchange: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * watching (int watching): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int exchange(StyxClient *client, Fcall *request, Fcall *reply, StyxCancel *cancel, int watching)
{
    client->attempted = 0;
    client->cancelled = 0;
    client->cycle = 0;
    if (client->failed)
    {
        return -1;
    }
    if (!client->mux)
    {
        int status = (cancel || watching) ? -1 : serialrpc(client, request, reply);
        if (client->failed)
        {
            styxclientemit(client, STYX_LINK_FAILED);
        }
        return status;
    }
    if (styxcancelled(cancel))
    {
        client->cancelled = 1;
        return -1;
    }
    unsigned length = convS2M(request, client->send, client->msize);
    StyxFlight *flight =
        length ? styxmuxsubmit(client->mux, client->send, length, client->msize, cancel, watching) : NULL;
    if (!flight)
    {
        if (!styxmuxalive(client->mux))
        {
            client->failed = 1;
        }
        return -1;
    }
    pthread_mutex_unlock(&client->lock);
    StyxAnswer answer = styxmuxcollect(flight);
    pthread_mutex_lock(&client->lock);
    client->attempted = answer.attempted;
    client->cancelled = answer.cancelled;
    client->cycle = answer.cycle;
    if (answer.cycle)
    {
        walk_note_cycle();
    }
    int status = answer.status;
    if (answer.length)
    {
        memcpy(client->receive, answer.bytes, answer.length);
        if (convM2S(client->receive, answer.length, reply) != answer.length)
        {
            styxrpcbad(client);
            status = -1;
        }
    }
    free(answer.bytes);
    if (!styxmuxalive(client->mux))
    {
        client->failed = 1;
    }
    return status;
}

/* EN/RU: See internal.h; ordinary native RPC retains its normal deadline.
 * См. internal.h; ordinary native RPC сохраняет обычный deadline. */


/* [standalone-contract:styxrpccancel]
 * EN: styxrpccancel: native client session operations and lifetime.
 * RU: styxrpccancel: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrpccancel(StyxClient *client, Fcall *request, Fcall *reply, StyxCancel *cancel)
{
    return exchange(client, request, reply, cancel, 0);
}

/* EN/RU: See internal.h; long-lived read still uses ordinary tag/flush fences.
 * См. internal.h; long-lived read использует обычные tag/flush fences. */


/* [standalone-contract:styxrpcwatch]
 * EN: styxrpcwatch: native client session operations and lifetime.
 * RU: styxrpcwatch: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrpcwatch(StyxClient *client, Fcall *request, Fcall *reply, StyxCancel *cancel)
{
    return exchange(client, request, reply, cancel, 1);
}

/* EN/RU: See internal.h; ordinary RPC uses native multiplexer when explicitly configured.
 * См. internal.h; обычный RPC использует явно настроенный native multiplexer. */


/* [standalone-contract:styxrpc]
 * EN: styxrpc: native client session operations and lifetime.
 * RU: styxrpc: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrpc(StyxClient *client, Fcall *request, Fcall *reply)
{
    return styxrpccancel(client, request, reply, NULL);
}

/* EN/RU: See internal.h; a semantic protocol failure also retires other admitted work.
 * См. internal.h; semantic protocol failure завершает другие admitted work. */


/* [standalone-contract:styxrpcbad]
 * EN: styxrpcbad: native client session operations and lifetime.
 * RU: styxrpcbad: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxrpcbad(StyxClient *client)
{
    client->failed = 1;
    if (client->mux)
    {
        styxmuxfail(client->mux);
    }
    else
    {
        styxclientemit(client, STYX_LINK_FAILED);
    }
}

/* EN/RU: See internal.h; errors cannot cause a second clunk or data replay.
 * См. internal.h; ошибка не вызывает повторный clunk/data replay. */


/* [standalone-contract:styxclunknumber]
 * EN: styxclunknumber: native client session operations and lifetime.
 * RU: styxclunknumber: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * number (u32int number): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclunknumber(StyxClient *client, u32int number)
{
    Fcall request, reply;
    memset(&request, 0, sizeof(request));
    request.type = Tclunk;
    request.fid = number;
    styxrpc(client, &request, &reply);
}

/* EN/RU: See client.h; no network I/O, only a local terminal-state snapshot.
 * См. client.h; без network I/O, только terminal-state snapshot. */


/* [standalone-contract:styxclientalive]
 * EN: styxclientalive: native client session operations and lifetime.
 * RU: styxclientalive: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientalive(StyxClient *client)
{
    if (!client)
    {
        return 0;
    }
    pthread_mutex_lock(&client->lock);
    int alive = !client->failed && (!client->mux || styxmuxalive(client->mux));
    pthread_mutex_unlock(&client->lock);
    return alive;
}

/* EN/RU: See client.h; the final reference closes transport without waiting for protocol acknowledgement.
 * См. client.h; последняя ссылка закрывает transport без ожидания protocol acknowledgement. */


/* [standalone-contract:styxclientfree]
 * EN: styxclientfree: native client session operations and lifetime.
 * RU: styxclientfree: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientfree(StyxClient *client)
{
    if (!client)
    {
        return;
    }
    pthread_mutex_lock(&client->lock);
    int last = --client->references == 0;
    pthread_mutex_unlock(&client->lock);
    if (last)
    {
        styxlifequiet(client->life);
        if (client->mux)
        {
            styxmuxfree(client->mux);
        }
        else if (!client->transportclosed)
        {
            client->transport.close(client->transport.state);
        }
        styxlifedrop(client->life);
        pthread_mutex_destroy(&client->lock);
        free(client->send);
        free(client->receive);
        free(client);
    }
}

/* EN/RU: Constructor failures consume native inputs, joining only in this explicit I/O path.
 * Constructor failure поглощает native inputs; join только в explicit I/O path. */


/* [standalone-contract:startfailed]
 * EN: startfailed: native client session operations and lifetime.
 * RU: startfailed: нативные client-операции session и время жизни.
 * transport (const StyxTransport *transport): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void startfailed(const StyxTransport *transport, StyxMux *mux)
{
    if (mux)
    {
        styxmuxshutdown(mux);
        styxmuxfree(mux);
    }
    else
    {
        transport->close(transport->state);
    }
}

/* EN/RU: See client.h; copy protocol output before the next frame replaces it.
 * См. client.h; скопировать protocol output до следующего frame. */
