/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of session.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* [standalone-contract:styxclientstart]
 * EN: styxclientstart: native client session operations and lifetime.
 * RU: styxclientstart: нативные client-операции session и время жизни.
 * transport (const StyxTransport *transport): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * msize (unsigned msize): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * user (const char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: StyxClient *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *
styxclientstart(const StyxTransport *transport, StyxMux *mux, unsigned msize, const char *user, const char *name)
{
    if (!mux && (!transport || !transport->read || !transport->write || !transport->close || !transport->begin))
    {
        return NULL;
    }
    if (msize < 256 || msize > 65536 || !user || !name || strlen(user) > 4095 || strlen(name) > 4095)
    {
        startfailed(transport, mux);
        return NULL;
    }
    StyxClient *client = calloc(1, sizeof(*client));
    if (!client)
    {
        startfailed(transport, mux);
        return NULL;
    }
    client->send = malloc(msize);
    client->receive = malloc(msize);
    if (!client->send || !client->receive || pthread_mutex_init(&client->lock, NULL))
    {
        free(client->send);
        free(client->receive);
        free(client);
        startfailed(transport, mux);
        return NULL;
    }
    client->life = mux ? styxmuxlife(mux) : styxlifecreate();
    if (!client->life || (mux && !styxliferetain(client->life)))
    {
        pthread_mutex_destroy(&client->lock);
        free(client->send);
        free(client->receive);
        free(client);
        startfailed(transport, mux);
        return NULL;
    }
    client->mux = mux;
    if (transport)
    {
        client->transport = *transport;
    }
    client->msize = msize;
    client->references = 1;
    client->lastfid = 1;
    Fcall request, reply;
    pthread_mutex_lock(&client->lock);
    memset(&request, 0, sizeof(request));
    request.type = Tversion;
    request.version = VERSION9P;
    request.msize = msize;
    if (styxrpc(client, &request, &reply) != 1 || reply.msize < 256 || reply.msize > msize ||
        GBIT32(client->receive) != 7 + BIT32SZ + BIT16SZ + 6 || strcmp(reply.version, VERSION9P))
    {
        pthread_mutex_unlock(&client->lock);
        styxclientshutdown(client);
        styxclientfree(client);
        return NULL;
    }
    client->msize = reply.msize;
    request.type = Tattach;
    request.fid = 1;
    request.afid = NOFID;
    request.uname = (char *)user;
    request.aname = (char *)name;
    if (styxrpc(client, &request, &reply) != 1)
    {
        pthread_mutex_unlock(&client->lock);
        styxclientshutdown(client);
        styxclientfree(client);
        return NULL;
    }
    client->root = reply.qid;
    pthread_mutex_unlock(&client->lock);
    return client;
}

/* EN/RU: See client.h; legacy transport keeps its existing serialized exchange policy.
 * См. client.h; legacy transport сохраняет serialized exchange policy. */


/* [standalone-contract:styxclientmount]
 * EN: styxclientmount: native client session operations and lifetime.
 * RU: styxclientmount: нативные client-операции session и время жизни.
 * transport (const StyxTransport *transport): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * msize (unsigned msize): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * user (const char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: StyxClient *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name)
{
    return styxclientstart(transport, NULL, msize, user, name);
}

/* EN/RU: See internal.h; preserve first legacy cause and admit callbacks outside client lock.
 * См. internal.h; сохранить first legacy cause и вызвать callbacks вне client lock. */


/* [standalone-contract:styxclientemit]
 * EN: styxclientemit: native client session operations and lifetime.
 * RU: styxclientemit: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * reason (int reason): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientemit(StyxClient *client, int reason)
{
    if (!client->terminal)
    {
        client->terminal = reason;
    }
    reason = client->terminal;
    pthread_mutex_unlock(&client->lock);
    styxlifeemit(client->life, reason);
    pthread_mutex_lock(&client->lock);
}

/* EN/RU: See client.h; source lifetime is retained by accepted watcher nodes.
 * См. client.h; source lifetime удерживается принятыми watcher nodes. */


/* [standalone-contract:styxclientwatch]
 * EN: styxclientwatch: native client session operations and lifetime.
 * RU: styxclientwatch: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * notify (StyxTerminal notify): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxWatch *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxWatch *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *))
{
    return client ? styxlifewatch(client->life, notify, state, release) : NULL;
}
